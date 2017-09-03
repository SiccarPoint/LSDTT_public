///=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
///
/// LSDRasterMaker.cpp
/// header for the RasterMaker object
/// The raster maker is a series of simple functions to make some rasters
/// with different properties. 
/// The initial use is mainly to make rasters for use in the raster model
/// for uplift and K
///=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
///
/// This object is written by
/// @author Simon M. Mudd, University of Edinburgh
///
///=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
///
/// Version 0.0.1    01/09/2017
///=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <ctime>
#include <cmath>
#include "TNT/tnt.h"
#include "LSDRaster.hpp"
#include "LSDRasterMaker.hpp"
#include "LSDStatsTools.hpp"
using namespace std;
using namespace TNT;

#ifndef LSDRasterMaker_CPP
#define LSDRasterMaker_CPP


void LSDRasterMaker::create()
{
  NRows = 100;
  NCols = 100;
  DataResolution = 10;
  NoDataValue = -9999;
  XMinimum = 0;
  YMinimum = 0;
  RasterData = Array2D <float> (NRows, NCols, 0.0);

  int zone = 1;
  string NorS = "N";
  impose_georeferencing_UTM(zone, NorS);
}

// this creates a raster using an infile
void LSDRasterMaker::create(string filename, string extension)
{
  read_raster(filename,extension);
}



void LSDRasterMaker::create(int NRows, int NCols)
{
  this->NRows = NRows;
  this->NCols = NCols;
  this->DataResolution = 10;
  this->NoDataValue = -9999;
  XMinimum = 0;
  YMinimum = 0;
  RasterData = Array2D <float> (NRows, NCols, 0.0);

  int zone = 1;
  string NorS = "N";
  impose_georeferencing_UTM(zone, NorS);
}


// this creates a LSDRasterModel raster from another LSDRaster
void LSDRasterMaker::create(LSDRaster& An_LSDRaster)
{
  NRows = An_LSDRaster.get_NRows();
  NCols = An_LSDRaster.get_NCols();
  XMinimum = An_LSDRaster.get_XMinimum();
  YMinimum = An_LSDRaster.get_YMinimum();
  DataResolution = An_LSDRaster.get_DataResolution();
  NoDataValue = An_LSDRaster.get_NoDataValue();
  GeoReferencingStrings =  An_LSDRaster.get_GeoReferencingStrings();
  RasterData = An_LSDRaster.get_RasterData();
}



// This returns the data in the raster model as a raster
LSDRaster LSDRasterMaker::return_as_raster()
{
  LSDRaster NewRaster(NRows, NCols, XMinimum, YMinimum,
                      DataResolution, NoDataValue, RasterData, 
                      GeoReferencingStrings);
  return NewRaster;
}
 
 
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// This resizes and resets the model
// This overloaded version also resets the data resolution
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void LSDRasterMaker::resize_and_reset( int new_rows, int new_cols, float new_resolution, float new_value )
{
  // set up some empty arrays
  Array2D<float> empty_array_sized(new_rows,new_cols,new_value);

  // reset the size of the RasterData
  RasterData = empty_array_sized.copy();

  // reset the rows and columns
  NRows = new_rows;
  NCols = new_cols;

  DataResolution = new_resolution;

  int zone = 1;
  string NorS = "N";
  impose_georeferencing_UTM(zone, NorS);
}
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Gets the minimum and maximum values in the raster
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
vector<float> LSDRasterMaker::minimum_and_maximum_value()
{

  // The vector min max will contain minimum and maximum values. It is initiated
  // with a very high minimum and a very low maximum to guarantee that one will 
  // always get sensible if the raster has non-nodata values.
  vector<float> min_max;
  min_max.push_back(1e12);
  min_max.push_back(-9998.0);
  
  for(int row = 0; row < NRows; row++)
  {
    for(int col = 0; col < NCols; col++)
    {
      if(RasterData[row][col] != NoDataValue)
      {
        if(RasterData[row][col] > min_max[1])
        {
          min_max[1]= RasterData[row][col];
        }
        if(RasterData[row][col] < min_max[0])
        {
          min_max[0]= RasterData[row][col];
        }
      }
    }
  }
  return min_max;

}



//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Some functions for making random values in the rasters
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void LSDRasterMaker::random_square_blobs(int minimum_blob_size, int maximum_blob_size, float minimum_value, float maximum_value, int n_blobs)
{
  long seed = time(NULL);
  
  // Lets make some blobs!!!
  for (int blob_n = 0; blob_n < n_blobs; blob_n++)
  {
    // get the centrepoint of the blob
    float row_frac = ran3(&seed);
    float col_frac = ran3(&seed);
    
    // get the row and column of the centre of the blob
    float frow = row_frac*float(NRows);
    int this_row = floor(frow);
    
    if (this_row < 0)
    {
      this_row = 0; 
    }
    if(this_row >= NRows)
    {
      this_row = NRows-1;
    }
    
    // get the row and column of the centre of the blob
    float fcol = col_frac*float(NCols);
    int this_col = floor(fcol);
    
    if (this_col < 0)
    {
      this_col = 0; 
    }
    if(this_col >= NCols)
    {
      this_col = NCols-1;
    }
    
    // Get the size of the blob. This will need to be odd
    int size_range =  maximum_blob_size-minimum_blob_size;
    float this_size;
    
    if(size_range == 0)
    {
      cout << "Check you prarameters, the size range is zero." << endl;
      this_size = minimum_blob_size;
    }
    else
    {
      this_size = ran3(&seed)*float(size_range)+float(minimum_blob_size);
    }
    
    int size = int(this_size);
    
    // get the starting rows and ending rows. Note that I am not being very careful about
    // this being exactly the right dimension 
    int start_row = this_row - size/2;
    int end_row = this_row + size/2;
    int start_col = this_col - size/2;
    int end_col = this_col + size/2;
    
    // get the new value
    float value_range = maximum_value-minimum_value;
    float this_blob_value;
    if(value_range == 0)
    {
      cout << "Check you prarameters, the value range is zero." << endl;
      this_blob_value = minimum_value;
    }
    else
    {
      this_blob_value = ran3(&seed)*value_range+minimum_value;
    }
    
    //cout << "This blob is: " << blob_n << " with a K of: " << this_blob_value << endl;
    //cout << "The size is: " << this_size << " or " << size <<endl;
    
    // now update the values
    for(int row = start_row; row<=end_row; row++)
    {
      for(int col = start_col; col<= end_col; col++)
      {
        
        if( row >= 0 && row<NRows && col >= 0 && col<NCols)
        {
          RasterData[row][col] = this_blob_value;
        }
      }
    }
  }
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Some functions for making random values in the rasters
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void LSDRasterMaker::sine_waves(vector<float> x_coefficients, vector<float> y_coefficients)
{
  int n_x_coeff = int(x_coefficients.size());
  int n_y_coeff = int(y_coefficients.size());
  
  float x_factor = M_PI/ float(NCols-1);
  float y_factor = M_PI / float(NRows-1);
  
  float this_x_value;
  float this_y_value;
  
  // so the wavelengths of the sin waves depend on the number
  for (int row = 0; row<NRows; row++)
  {
    for(int col = 0; col<NCols; col++)
    {
      this_x_value = 0;
      for(int xv = 0; xv<n_x_coeff; xv++)
      {
        this_x_value+=x_coefficients[xv]*sin(x_factor*(xv+1)*float(col));
      }
      this_y_value = 0;
      for(int yv = 0; yv<n_y_coeff; yv++)
      {
        this_y_value+=y_coefficients[yv]*sin(y_factor*(yv+1)*float(row));
      }
      RasterData[row][col] = this_x_value+this_y_value;
    }
  }
}



#endif
