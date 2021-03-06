#ifndef DATA_MATRIX_H
#define DATA_MATRIX_H

// Copyright 2013 Tom SF Haines

// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

//   http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.



#include <Python.h>
#include <structmember.h>

#ifndef __APPLE__
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#endif
#include <numpy/arrayobject.h>

#include "philox.h"
#include "convert.h"

// Provides a wrapper around a numpy matrix that converts it into a data matrix of floats - allows each index of the dat matrix to be assigned to one of three classes - data, feature or spatial. It does linearisation to allow indexing of the exemplars/features, always in row major order...



// Define the types for an index of a matrix...
typedef enum {DIM_DATA, DIM_DUAL, DIM_FEATURE} DimType;

// A function that converts a pointer to a float...
typedef float (*ToFloat)(void * data);

// Helper function, that is also used elsewhere in the system...
ToFloat KindToFunc(const PyArray_Descr * descr);

// An internal structure that is used by the system...
typedef struct ConvertOp ConvertOp;

struct ConvertOp
{
 const Convert * conv;
 int offset_external; // Offset of external representation.
 int offset_internal; // Offset of internal representation.
};



// Define the structure that wraps a matrix...
typedef struct DataMatrix DataMatrix;

struct DataMatrix
{
 // Actual data matrix...
  PyArrayObject * array;
  
 // Assignment of type to each dimension of the dm...
  DimType * dt;
  
 // The index of the feature that provides the weight, negative if none; also the multiplier of the weight to output...
  int weight_index;
  float weight_scale;
  
 // Cumulative weight array, so we can do a binary search when drawing a  specific exemplar from the data matrix - only used if there is a weight_index set. If not NULL then it must be valid - note that this uses a random weight_scale as its not relevant (Whatever it was when the cache is built), and is inclusive, so the last entry is the total weight...
  float * weight_cum;
  
 // Number of data items represented by the data matrix - calculated when you set the array...
  int exemplars;
  
 // Number of features for each exemplar - calculated when you set the array as its quite complex (Takes into account feature dimensions and the weight feature)...
  int feats;
  
 // Number of dual features - avoids having to calculate it at every request...
  int dual_feats;
  
 // Entries in the feature vector are multiplied by these values before extraction...
  float * mult;
  
 // Temporary storage - when you request a feature vector this is what it is returned in...
  float * fv;
  
 // Optimisation structure for the feature extraction - how many feature dimensions exist, and their indices...
  int feat_dims;
  int * feat_indices;
  
 // Function pointer to accelerate conversion...
  ToFloat to_float;

 // All the stuff for conversion - only valid if the fv_conv pointer is not null...
  int feats_conv; // Set to feats even when conversion system not active, to avoid branching.
  float * fv_conv; // Substitute for fv, after conversion to internal use state.
  
  int ops_conv; // Number of below - conversion is to loop and apply each in turn.
  ConvertOp * conv;
};



// For initialising the pointers within the data matrix object, and deinitialising them when done...
void DataMatrix_init(DataMatrix * dm);
void DataMatrix_deinit(DataMatrix * dm);

// Allows you to set a numpy data matrix for use; you have to assign a type to each dimension as well. If you want a weight that is not 1 you can assign one of the features to be a weight, by giving its index - this feature will not appear in the feature count and return of the fv method. Otherwise set the weight index to be negative. You can optionally provide a conversion string (rather than NULL), of convert codes to define a runtime format different to the storage format (no error checking!)...
void DataMatrix_set(DataMatrix * dm, PyArrayObject * array, DimType * dt, int weight_index, const char * conv_str);


// Returns basic stats...
int DataMatrix_exemplars(DataMatrix * dm);
int DataMatrix_features(DataMatrix * dm);

int DataMatrix_ext_features(DataMatrix * dm); // External feature count, before conversion (if any).


// Allows you to set the multipliers for the features - the scale array passed in better have the right length, as returned by the feats method. You should also provide a weight_scale...
void DataMatrix_set_scale(DataMatrix * dm, float * scale, float weight_scale);


// Fetches a feature vector, using a single index to do row-major indexing into all dimensions marked as data or dual. Note that the returned pointer is to internal storage, that is replaced every time this method is called. The dual dimensions will always be first, followed by all the feature dimensions in row major flattened order. If you want the weight as well provide a pointer and it will be filled...
float * DataMatrix_fv(DataMatrix * dm, int index, float * weight);

// As above, but in the external format, without conversion or scaling...
float * DataMatrix_ext_fv(DataMatrix * dm, int index, float * weight);

// Draws the index of a random exemplar from the datamatrix, using the philox random number generator (If exemplars are weighted it will be a weighted draw)...
int DataMatrix_draw(DataMatrix * dm, PhiloxRNG * rng);


// Converts an external vector to an internal vector, noting that if no conversion happens it just returns the pointer to the provided external array rather than copying the data across (in the no-conversion case its safe for internal to be NULL!) This means conversion and multiplication by scale. Effectivly destructive to external...
float * DataMatrix_to_int(DataMatrix * dm, float * external, float * internal);

// As above, other direction...
float * DataMatrix_to_ext(DataMatrix * dm, float * internal, float * external);


// Returns how many bytes are consumed by the DataMatrix object, excluding the numpy array it points at...
size_t DataMatrix_byte_size(DataMatrix * dm);



#endif
