
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// LSDFloodplain.cpp
//
// Land Surface Dynamics Floodplain Object
//
// This object creates and stores information about floodplain and terraces
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// An object within the University
//  of Edinburgh Land Surface Dynamics group topographic toolbox
//
// Developed by:
//  Simon M. Mudd
//  Martin D. Hurst
//  David T. Milodowski
//  Stuart W.D. Grieve
//  Declan A. Valters
//  Fiona Clubb
//
// Copyright (C) 2013 Simon M. Mudd 2013
//
// Developer can be contacted by simon.m.mudd _at_ ed.ac.uk
//
//    Simon Mudd                                                    
//    University of Edinburgh
//    School of GeoSciences
//    Drummond Street
//    Edinburgh, EH8 9XP
//    Scotland
//    United Kingdom
//
// This program is free software;
// you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation;
// either version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY;
// without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the
// GNU General Public License along with this program;
// if not, write to:
// Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor,
// Boston, MA 02110-1301
// USA
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#ifndef LSDFloodplain_CPP
#define LSDFloodplain_CPP

#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <math.h>
#include "TNT/tnt.h"
#include "LSDFlowInfo.hpp"
#include "LSDRaster.hpp"
#include "LSDIndexRaster.hpp"
#include "LSDIndexChannel.hpp"
#include "LSDJunctionNetwork.hpp"
#include "LSDStatsTools.hpp"
#include "LSDFloodplain.hpp"
using namespace std;
using namespace TNT;

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// create
// this populates the binary array and connected components array for the floodplain
// given rasters of channel relief and slope and thresholds for both. Each pixel
// must be below the slope and channel relief threshold to be classified as floodplain.
// User must set a minimum floodplain patch size (in pixels, set to 0 if all patches are kept).
//
// FJC 18/10/16
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void LSDFloodplain::create(LSDRaster& ChannelRelief, LSDRaster& Slope, float relief_thresh, float slope_thresh, int min_patch_size)
{	
	
  /// set the protected variables
	NRows = ChannelRelief.get_NRows();
	NCols = ChannelRelief.get_NCols();
	XMinimum = ChannelRelief.get_XMinimum();
	YMinimum = ChannelRelief.get_YMinimum();
	DataResolution = ChannelRelief.get_DataResolution();
	NoDataValue = ChannelRelief.get_NoDataValue();
	GeoReferencingStrings = ChannelRelief.get_GeoReferencingStrings();
	
	relief_threshold = relief_thresh;
	slope_threshold = slope_thresh;
	
	//declare the arrays
	Array2D<int> TempBinArray(NRows,NCols,0);
	Array2D<int> TempLinkArray(NRows,NCols,NoDataValue);
	BinaryArray = TempBinArray.copy();
	ConnectedComponents_Array = TempLinkArray.copy();
	
	//loop through every row and col and get the slope and relief values
  for (int i =0; i < NRows; i++)
  {
    for (int j = 0; j < NCols; j++)
    {
      if (ChannelRelief.get_data_element(i,j) != NoDataValue && Slope.get_data_element(i,j) != NoDataValue)
      {
        float slope = Slope.get_data_element(i,j);
        float relief = ChannelRelief.get_data_element(i,j);
        if (relief < relief_threshold && slope < slope_threshold)        //floodplain points must be lower than both the relief
        {                                                                //and the slope threshold.
          BinaryArray[i][j] = 1;
        }
      }
    }
  }
	LSDIndexRaster FloodplainRaster(NRows,NCols,XMinimum,YMinimum,DataResolution,NoDataValue,BinaryArray,GeoReferencingStrings);
	
	// run the connected components algorithm on the floodplain array
	LSDIndexRaster ConnectedComponents = FloodplainRaster.ConnectedComponents();
	if (min_patch_size > 0)
	{
		LSDIndexRaster ConnectedComponents_final = ConnectedComponents.RemoveSmallPatches(min_patch_size);
		ConnectedComponents_Array = ConnectedComponents_final.get_RasterData();
	}
	else
	{		
		ConnectedComponents_Array = ConnectedComponents.get_RasterData();	
	}
	
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Takes in a junction number and generates the main stem channel from this point
// The information about each floodplain or terrace pixel is then calculated relative
// to the main channel.
// FJC 18/10/16
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void LSDFloodplain::get_main_stem_information(int junction_number, LSDJunctionNetwork& ChanNetwork, LSDFlowInfo& FlowInfo, LSDRaster& DistFromOutlet, LSDRaster& ElevationRaster)
{
	// set protected variables
	JunctionNumber = junction_number;
	
	//set up the emtpy arrays
	Array2D<int> TempIntArray(NRows,NCols,NoDataValue);
	Array2D<float> TempFloatArray(NRows,NCols,NoDataValue);
	MainStemNIs = TempIntArray.copy();
	ChannelRelief_array = TempFloatArray.copy();
	UpstreamDistance_array = TempFloatArray.copy();
	FlowLength_array = TempFloatArray.copy();
		
	// get the main stem channel from this junction
	LSDIndexChannel MainStem = ChanNetwork.generate_longest_index_channel_from_junction(JunctionNumber, FlowInfo, DistFromOutlet);
	int downstream_node = ChanNetwork.get_Node_of_Junction(JunctionNumber);
	UpslopeNodes = FlowInfo.get_upslope_nodes(downstream_node);
	cout << "There are: " << UpslopeNodes.size() << " nodes upslope of this junction." << endl;
	
	// loop through all the upslope nodes and find ones that are in the connected components raster
	for (int i = 0; i < int(UpslopeNodes.size()); i++)
	{
		cout << flush << "Node = " << i+1 << " of " << UpslopeNodes.size() << "\r";
		int row, col;		
		FlowInfo.retrieve_current_row_and_col(UpslopeNodes[i], row, col);
		//cout << ConnectedComponents_Array[row][col] << endl;
		if (ConnectedComponents_Array[row][col] != NoDataValue)
		{
			int ChannelNode;
			float FlowLength, DistanceUpstream, Relief;
			ChanNetwork.get_info_nearest_channel_to_node_main_stem(UpslopeNodes[i], FlowInfo, ElevationRaster, DistFromOutlet, MainStem, ChannelNode, FlowLength, DistanceUpstream, Relief);
			// populate arrays with relief and distance
			MainStemNIs[row][col] = ChannelNode;
			ChannelRelief_array[row][col] = Relief;
			UpstreamDistance_array[row][col] = DistanceUpstream;
			FlowLength_array[row][col] = FlowLength;
			//cout << "Relief: " << Relief << " Upstream dist: " << DistanceUpstream << endl;			
			CCNodes.push_back(UpslopeNodes[i]);
			UpstreamDistances.push_back(DistanceUpstream);
			FlowLengths.push_back(FlowLength);
			ChannelReliefs.push_back(Relief);
			MainStemNodes.push_back(ChannelNode);
		}
	}		
}

//----------------------------------------------------------------------------------------
// This function separates the floodplain into floodplain patches and terrace patches depending
// on whether they are connected to the channel network.
// FJC 18/10/16
//---------------------------------------------------------------------------------------- 
void LSDFloodplain::separate_floodplain_and_terrace_patches(LSDJunctionNetwork& ChanNetwork, LSDFlowInfo& FlowInfo, float threshold_SO, LSDIndexRaster& FloodplainPatches, LSDIndexRaster& TerracePatches)
{
  Array2D<int> FloodplainPatches_array(NRows,NCols,NoDataValue);
	Array2D<int> TerracePatches_array(NRows,NCols,NoDataValue);
  vector<int> patch_ids_channel;
	Array2D<int> StreamOrderArray = ChanNetwork.get_StreamOrderArray();
  
  //loop through the DEM and get the ID of all patches connected to the channel network
  for (int row = 0; row < NRows; row++)
  {
    for (int col = 0; col < NCols; col++)
    {
      if (ConnectedComponents_Array[row][col] != NoDataValue)
      {
      //check if the pixel is part of the channel network
        if (StreamOrderArray[row][col] >= threshold_SO)
        {
          patch_ids_channel.push_back(ConnectedComponents_Array[row][col]);
        }
      }
    }
  }
  
  //for each pixel, find if it is in a patch with an ID in patch_ids_channel vector
  vector<int>::iterator find_it;
  for (int row = 0; row < NRows; row++)
  {
    for (int col = 0; col < NCols; col++)
    {
      if (ConnectedComponents_Array[row][col] != NoDataValue)
      {
				int ThisNode = FlowInfo.retrieve_node_from_row_and_column(row, col);
        int patch_id = ConnectedComponents_Array[row][col];
        find_it = find(patch_ids_channel.begin(), patch_ids_channel.end(), patch_id);   //search ID vector for patch ID of pixel
        if (find_it != patch_ids_channel.end())
        {
          FloodplainPatches_array[row][col] = patch_id;   
					FloodplainNodes.push_back(ThisNode);
        }
				else
				{
					TerracePatches_array[row][col] = patch_id;
					TerraceNodes.push_back(ThisNode);
				}
      }      
    }
  }
  
  //get the LSDIndexRaster from floodplain patches array
  LSDIndexRaster FloodplainPatches_temp(NRows,NCols, XMinimum, YMinimum, DataResolution, NoDataValue, FloodplainPatches_array, GeoReferencingStrings);
	
	LSDIndexRaster TerracePatches_temp(NRows,NCols, XMinimum, YMinimum, DataResolution, NoDataValue, TerracePatches_array, GeoReferencingStrings);
	
	//copy to rasters
	FloodplainPatches = FloodplainPatches_temp;
	TerracePatches = TerracePatches_temp;
}

//----------------------------------------------------------------------------------------
// FUNCTIONS TO GENERATE RASTERS
//---------------------------------------------------------------------------------------- 

//----------------------------------------------------------------------------------------
// Get the raster of channel relief relative to main stem
// FJC 18/10/16
//---------------------------------------------------------------------------------------- 
LSDRaster LSDFloodplain::print_ChannelRelief_to_Raster()
{
	LSDRaster ChannelRelief(NRows,NCols, XMinimum, YMinimum, DataResolution, NoDataValue, ChannelRelief_array, GeoReferencingStrings);
	return ChannelRelief;
}

//----------------------------------------------------------------------------------------
// Get the raster of upstream distance relative to main stem
// FJC 18/10/16
//---------------------------------------------------------------------------------------- 
LSDRaster LSDFloodplain::print_UpstreamDistance_to_Raster()
{
	LSDRaster UpstreamDist(NRows,NCols, XMinimum, YMinimum, DataResolution, NoDataValue, UpstreamDistance_array, GeoReferencingStrings);
	return UpstreamDist;
}

//----------------------------------------------------------------------------------------
// Get the raster of flow lengths relative to main stem
// FJC 18/10/16
//---------------------------------------------------------------------------------------- 
LSDRaster LSDFloodplain::print_FlowLengths_to_Raster()
{
	LSDRaster FlowLengths(NRows,NCols, XMinimum, YMinimum, DataResolution, NoDataValue, FlowLength_array, GeoReferencingStrings);
	return FlowLengths;
}

//----------------------------------------------------------------------------------------
// FUNCTIONS TO PRINT TEXT FILES
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
// Write a text file with the distance along main stem and channel relief for each 
// CC pixel.  The format is:
// distance_upstream channel_relief
// FJC 19/10/16
//---------------------------------------------------------------------------------------- 
void LSDFloodplain::print_ChannelRelief_to_File(string filename)
{
	ofstream output_file;
	output_file.open(filename.c_str());
	
	for (int i = 0; i < int(CCNodes.size()); i++)
	{
		output_file << UpstreamDistances[i] << " " << ChannelReliefs[i] << endl;
	}
	
	output_file.close();
}

//----------------------------------------------------------------------------------------
// Write a text file with the distance along main stem and channel relief for each 
// terrace pixel. The pixels must first be separated into floodplain and terrace nodes using
// the function separate_floodplain_and_terrace_patches
// The format is:
// distance_upstream channel_relief
// FJC 20/10/16
//---------------------------------------------------------------------------------------- 
void LSDFloodplain::print_Terrace_ChannelRelief_to_File(string filename)
{
	ofstream output_file;
	output_file.open(filename.c_str());
	vector<int>::iterator find_it;
	
	for (int i = 0; i < int(TerraceNodes.size()); i++)
	{
		// find the corresponding distance and channel relief in the vectors
		find_it = find(CCNodes.begin(), CCNodes.end(), TerraceNodes[i]);
		if (find_it != CCNodes.end())
		{
			int index = find_it - CCNodes.begin();
			output_file << UpstreamDistances[index] << " " << ChannelReliefs[index] << endl;
		}		
	}
	
	output_file.close();
}

//----------------------------------------------------------------------------------------
// Write a text file with the distance along main stem and channel relief for each 
// CC pixel, binned by distance along main stem.  The user specifies the 
// bin width. The format is:
// distance_upstream channel_relief
// FJC 20/10/16
//---------------------------------------------------------------------------------------- 
void LSDFloodplain::print_Binned_ChannelRelief_to_File(string filename, float& bin_width, float& bin_lower_limit, float& bin_threshold)
{
	// declare vectors for binning
	vector<float> MeanDistances, MeanReliefs, Midpoints_distance, MedianReliefs, StDev_distance, StDev_relief, StErr_distance, StErr_relief;
	vector<int> n_observations;
	
	cout << "\t Binning, there are " << UpstreamDistances.size() << " observations" << endl;
	
	// bin the data
	bin_data(UpstreamDistances, ChannelReliefs, bin_width, MeanDistances, MeanReliefs, Midpoints_distance, MedianReliefs, StDev_distance, StDev_relief, StErr_distance, StErr_relief, n_observations, bin_lower_limit, NoDataValue);
	
	cout << "\t Binned the data" << endl;
	
	RemoveSmallBins(MeanDistances, MeanReliefs, Midpoints_distance, StDev_distance, StDev_relief, StErr_distance, StErr_relief, n_observations, bin_threshold);
	
	cout << "\t Removed small bins" << endl;
	
	// write to file
	ofstream output_file;
	output_file.open(filename.c_str());
	
	for (int i = 0; i < int(MeanDistances.size()); i++)
	{
		output_file << MeanDistances[i] << " " << StDev_distance[i] << " " << StErr_distance[i] << " " << MeanReliefs[i] << " " << StDev_relief[i] << " " << StErr_relief[i] << endl;
	}
	
	output_file.close();
}

//----------------------------------------------------------------------------------------
// Write a text file with the distance along main stem and channel relief for each 
// terrace pixel, binned by distance along main stem.  The user specifies the 
// bin width. The format is:
// distance_upstream channel_relief
// FJC 20/10/16
//---------------------------------------------------------------------------------------- 
void LSDFloodplain::print_Binned_Terrace_ChannelRelief_to_File(string filename, float& bin_width, float& bin_lower_limit, float& bin_threshold)
{
	// get the upstream distances and channel relief of the terrace pixels
	vector<int>::iterator find_it;	
	vector<float> TerraceDistances, TerraceRelief;
	
	for (int i = 0; i < int(TerraceNodes.size()); i++)
	{
		// find the corresponding distance and channel relief in the vectors
		find_it = find(CCNodes.begin(), CCNodes.end(), TerraceNodes[i]);
		if (find_it != CCNodes.end())
		{
			int index = find_it - CCNodes.begin();
			TerraceDistances.push_back(UpstreamDistances[index]);
			TerraceRelief.push_back(ChannelReliefs[index]);
		}		
	}
	
	// declare vectors for binning
	vector<float> MeanDistances, MeanReliefs, Midpoints_distance, MedianReliefs, StDev_distance, StDev_relief, StErr_distance, StErr_relief;
	vector<int> n_observations;
	
	cout << "\t Binning, there are " << UpstreamDistances.size() << " observations" << endl;
	
	// bin the data
	bin_data(TerraceDistances, TerraceRelief, bin_width, MeanDistances, MeanReliefs, Midpoints_distance, MedianReliefs, StDev_distance, StDev_relief, StErr_distance, StErr_relief, n_observations, bin_lower_limit, NoDataValue);
	
	cout << "\t Binned the data" << endl;
	
	RemoveSmallBins(MeanDistances, MeanReliefs, Midpoints_distance, StDev_distance, StDev_relief, StErr_distance, StErr_relief, n_observations, bin_threshold);
	
	cout << "\t Removed small bins" << endl;
	
	// write to file
	ofstream output_file;
	output_file.open(filename.c_str());
	
	for (int i = 0; i < int(MeanDistances.size()); i++)
	{
		output_file << MeanDistances[i] << " " << StDev_distance[i] << " " << StErr_distance[i] << " " << MeanReliefs[i] << " " << StDev_relief[i] << " " << StErr_relief[i] << endl;
	}
	
	output_file.close();
}

#endif