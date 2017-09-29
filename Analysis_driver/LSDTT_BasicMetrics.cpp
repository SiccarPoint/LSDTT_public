//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// LSDTT_BasicMetrics.cpp
//
// This is a program for calculating basic landscape metrics.
// It includes options for slope, curvature, drainage area, and other metrics
//
// This program takes two arguments, the path name and the driver name
//
// The documentation is here:
// https://lsdtopotools.github.io/LSDTopoTools_ChiMudd2014/
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// Copyright (C) 2017 Simon M. Mudd 2017
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
// either version 3 of the License, or (at your option) any later version.
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
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <sys/time.h>
#include <fstream>
#include "../LSDStatsTools.hpp"
#include "../LSDRaster.hpp"
#include "../LSDRasterInfo.hpp"
#include "../LSDIndexRaster.hpp"
#include "../LSDFlowInfo.hpp"
#include "../LSDJunctionNetwork.hpp"
#include "../LSDIndexChannelTree.hpp"
#include "../LSDParameterParser.hpp"
#include "../LSDSpatialCSVReader.hpp"
#include "../LSDShapeTools.hpp"

int main (int nNumberofArgs,char *argv[])
{

  //Test for correct input arguments
  if (nNumberofArgs!=3)
  {
    cout << "=========================================================" << endl;
    cout << "|| Welcome to the LSDTopoTools basic metrics tool!     ||" << endl;
    cout << "|| This program has a number of options for calculating||" << endl;
    cout << "|| simple landscape metrics.                           ||" << endl;
    cout << "|| This program was developed by Simon M. Mudd         ||" << endl;
    cout << "||  at the University of Edinburgh                     ||" << endl;
    cout << "=========================================================" << endl;
    cout << "This program requires two inputs: " << endl;
    cout << "* First the path to the parameter file." << endl;
    cout << "* Second the name of the param file (see below)." << endl;
    cout << "---------------------------------------------------------" << endl;
    cout << "Then the command line argument will be: " << endl;
    cout << "In linux:" << endl;
    cout << "./LSDTT_BasicMetrics.exe /LSDTopoTools/Topographic_projects/LSDTT_chi_examples/ Xian_example1.driver" << endl;
    cout << "=========================================================" << endl;
    exit(EXIT_SUCCESS);
  }

  string path_name = argv[1];
  string f_name = argv[2];

  // load parameter parser object
  LSDParameterParser LSDPP(path_name,f_name);

  // for the chi tools we need georeferencing so make sure we are using bil format
  LSDPP.force_bil_extension();

  // maps for setting default parameters
  map<string,int> int_default_map;
  map<string,float> float_default_map;
  map<string,bool> bool_default_map;
  map<string,string> string_default_map;

  // Basic DEM preprocessing
  float_default_map["minimum_elevation"] = 0.0;
  float_default_map["maximum_elevation"] = 30000;
  float_default_map["min_slope_for_fill"] = 0.0001;
  bool_default_map["raster_is_filled"] = false; // assume base raster is already filled
  bool_default_map["remove_seas"] = true; // elevations above minimum and maximum will be changed to nodata
  string_default_map["CHeads_file"] = "NULL";
  bool_default_map["only_check_parameters"] = false;
  
  // the most basic raster printing
  bool_default_map["write_hillshade"] = false;
  bool_default_map["print_raster_without_seas"] = false;
  
  // Slope calculations
  float_default_map["surface_fitting_radius"] = 30;
  bool_default_map["print_smoothed_elevation"]= false;
  bool_default_map["print_slope"] = false;
  bool_default_map["print_aspect"]= false;
  bool_default_map["print_curvature"]= false;
  bool_default_map["print_planform_curvature"]= false;
  bool_default_map["print_profile_curvature"]= false;
  bool_default_map["print_tangential_curvature"]= false;
  bool_default_map["print_point_classification"]= false;
  

  // filling and drainage area
  bool_default_map["print_dinf_drainage_area_raster"] = false;
  bool_default_map["print_d8_drainage_area_raster"] = false;
  bool_default_map["print_QuinnMD_drainage_area_raster"] = false;
  bool_default_map["print_FreemanMD_drainage_area_raster"] = false;
  bool_default_map["print_MD_drainage_area_raster"] = false;
  bool_default_map["print_distance_from_outlet"] = false;
  bool_default_map["print_fill_raster"] = false;

  // Basic channel network
  int_default_map["threshold_contributing_pixels"] = 500;
  bool_default_map["print_stream_order_raster"] = false;
  bool_default_map["print_channels_to_csv"] = false;
  bool_default_map["print_junction_index_raster"] = false;
  bool_default_map["print_junctions_to_csv"] = false;
  
  // This converts all csv files to geojson (for easier loading in a GIS)
  bool_default_map["convert_csv_to_geojson"] = false;  

  // Use the parameter parser to get the maps of the parameters required for the
  // analysis
  LSDPP.parse_all_parameters(float_default_map, int_default_map, bool_default_map,string_default_map);
  map<string,float> this_float_map = LSDPP.get_float_parameters();
  map<string,int> this_int_map = LSDPP.get_int_parameters();
  map<string,bool> this_bool_map = LSDPP.get_bool_parameters();
  map<string,string> this_string_map = LSDPP.get_string_parameters();

  // Now print the parameters for bug checking
  cout << "PRINT THE PARAMETERS..." << endl;
  LSDPP.print_parameters();

  // location of the files
  string DATA_DIR =  LSDPP.get_read_path();
  string DEM_ID =  LSDPP.get_read_fname();
  string OUT_DIR = LSDPP.get_write_path();
  string OUT_ID = LSDPP.get_write_fname();
  string raster_ext =  LSDPP.get_dem_read_extension();
  vector<string> boundary_conditions = LSDPP.get_boundary_conditions();
  string CHeads_file = LSDPP.get_CHeads_file();

  cout << "Read filename is: " <<  DATA_DIR+DEM_ID << endl;
  cout << "Write filename is: " << OUT_DIR+OUT_ID << endl;

    // check to see if the raster exists
  LSDRasterInfo RI((DATA_DIR+DEM_ID), raster_ext);

  // load the  DEM
  LSDRaster topography_raster;
  if (this_bool_map["remove_seas"])
  {
    cout << "I am removing high and low values to get rid of things that should be nodata." << endl;
    LSDRaster start_raster((DATA_DIR+DEM_ID), raster_ext);
    // now get rid of the low and high values
    float lower_threshold = this_float_map["minimum_elevation"];
    float upper_threshold = this_float_map["maximum_elevation"];
    bool belowthresholdisnodata = true;
    LSDRaster Flooded = start_raster.mask_to_nodata_using_threshold(lower_threshold,belowthresholdisnodata);
    belowthresholdisnodata = false;
    topography_raster = Flooded.mask_to_nodata_using_threshold(upper_threshold,belowthresholdisnodata);

    if (this_bool_map["print_raster_without_seas"])
    {
      cout << "I'm replacing your raster with a raster without seas." << endl;
      string this_raster_name = OUT_DIR+OUT_ID;
      topography_raster.write_raster(this_raster_name,raster_ext);
    }
  }
  else
  {
    LSDRaster start_raster((DATA_DIR+DEM_ID), raster_ext);
    topography_raster = start_raster;
  }
  cout << "Got the dem: " <<  DATA_DIR+DEM_ID << endl;

  if(this_bool_map["only_check_parameters"])
  {
    cout << "You set the only_check_parameters flag to true; I have only printed" << endl;
    cout << "the parameters to file and am now exiting." << endl;
    exit(0);
  }


  //============================================================================
  // Compute hillshade and print
  //============================================================================
  if (this_bool_map["write_hillshade"])
  {
    cout << "Let me print the hillshade for you. " << endl;
    float hs_azimuth = 315;
    float hs_altitude = 45;
    float hs_z_factor = 1;
    LSDRaster hs_raster = topography_raster.hillshade(hs_altitude,hs_azimuth,hs_z_factor);

    string hs_fname = OUT_DIR+OUT_ID+"_hs";
    hs_raster.write_raster(hs_fname,raster_ext);
  }

  //============================================================================
  // The surface fitting metrics
  //============================================================================
  vector<int> raster_selection(8, 0);  // This controls which usrface fitting metrics to compute
  if(this_bool_map["print_smoothed_elevation"])
  {
    raster_selection[0] = 1;
  }
  if(this_bool_map["print_slope"])
  {
    raster_selection[1] = 1;
  }
  if(this_bool_map["print_aspect"])
  {
    raster_selection[2] = 1;
  }
  if(this_bool_map["print_curvature"])
  {
    raster_selection[3] = 1;
  }
  if(this_bool_map["print_planform_curvature"])
  {
    raster_selection[4] = 1;
  }
  if(this_bool_map["print_profile_curvature"])
  {
    raster_selection[5] = 1;
  }
  if(this_bool_map["print_tangential_curvature"])
  {
    raster_selection[6] = 1;
  }
  if(this_bool_map["print_point_classification"])
  {
    raster_selection[7] = 1;
  }
  vector<LSDRaster> surface_fitting;
  surface_fitting = topography_raster.calculate_polyfit_surface_metrics(this_float_map["surface_fitting_radius"], raster_selection);
  if(this_bool_map["print_smoothed_elevation"])
  {
    cout << "Let me print the smoothed elevation raster for you."  << endl;
    string this_raster_name = OUT_DIR+OUT_ID+"_SMOOTH";
    surface_fitting[0].write_raster(this_raster_name,raster_ext);
  }
  if(this_bool_map["print_slope"])
  {
    cout << "Let me print the slope raster for you."  << endl;
    string this_raster_name = OUT_DIR+OUT_ID+"_SLOPE";
    surface_fitting[1].write_raster(this_raster_name,raster_ext);
  }
  if(this_bool_map["print_aspect"])
  {
    cout << "Let me print the aspect raster for you."  << endl;
    string this_raster_name = OUT_DIR+OUT_ID+"_ASPECT";
    surface_fitting[2].write_raster(this_raster_name,raster_ext);
  }
  if(this_bool_map["print_curvature"])
  {
    cout << "Let me print the curvature raster for you."  << endl;
    string this_raster_name = OUT_DIR+OUT_ID+"_CURV";
    surface_fitting[3].write_raster(this_raster_name,raster_ext);
  }
  if(this_bool_map["print_planform_curvature"])
  {
    cout << "Let me print the planform curvature raster for you."  << endl;
    string this_raster_name = OUT_DIR+OUT_ID+"_PLFMCURV";
    surface_fitting[4].write_raster(this_raster_name,raster_ext);
  }
  if(this_bool_map["print_profile_curvature"])
  {
    cout << "Let me print the profile curvature raster for you."  << endl;
    string this_raster_name = OUT_DIR+OUT_ID+"_PROFCURV";
    surface_fitting[5].write_raster(this_raster_name,raster_ext);
  }
  if(this_bool_map["print_tangential_curvature"])
  {
    cout << "Let me print the tangential curvature raster for you."  << endl;
    string this_raster_name = OUT_DIR+OUT_ID+"_TANCURV";
    surface_fitting[6].write_raster(this_raster_name,raster_ext);
  }
  if(this_bool_map["print_point_classification"])
  {
    cout << "Let me print the point classification curvature raster for you."  << endl;
    string this_raster_name = OUT_DIR+OUT_ID+"_CLASS";
    surface_fitting[7].write_raster(this_raster_name,raster_ext);
  }
  


  //============================================================================
  //
  // EVERTHING BELOW THIS POINT NEEDS A FILL RASTER AND FLOW ROUTING
  // THIS IS WHERE MEMORY CONSUMPTION BECOMES A PROBLEM
  // The LSDFlowInfo object is ~10-20x as big as the original DEM so 
  // if the DEM is really big or you are working on a computer 
  // with limited memory you might get segmentation faults after this point
  //
  //============================================================================
  bool_default_map["print_dinf_drainage_area_raster"] = false;
  bool_default_map["print_d8_drainage_area_raster"] = false;
  bool_default_map["print_QuinnMD_drainage_area_raster"] = false;
  bool_default_map["print_FreemanMD_drainage_area_raster"] = false;
  bool_default_map["print_MD_drainage_area_raster"] = false;
  bool_default_map["print_fill_raster"] = false;

  // Basic channel network
  bool_default_map["print_stream_order_raster"] = false;
  bool_default_map["print_channels_to_csv"] = false;
  bool_default_map["print_junction_index_raster"] = false;
  bool_default_map["print_junctions_to_csv"] = false;
  
  if (this_bool_map["print_dinf_drainage_area_raster"]
        || this_bool_map["print_d8_drainage_area_raster"]
        || this_bool_map["print_QuinnMD_drainage_area_raster"]
        || this_bool_map["print_FreemanMD_drainage_area_raster"]
        || this_bool_map["print_MD_drainage_area_raster"]
        || this_bool_map["print_fill_raster"]
        || this_bool_map["print_stream_order_raster"]
        || this_bool_map["print_channels_to_csv"]
        || this_bool_map["print_junction_index_raster"]
        || this_bool_map["print_junctions_to_csv"])
  {
        
        
    //==========================================================================
    // Fill the raster
    //==========================================================================
    LSDRaster filled_topography;
    if ( this_bool_map["raster_is_filled"] )
    {
      cout << "You have chosen to use a filled raster." << endl;
      filled_topography = topography_raster;
    }
    else
    {
      cout << "Let me fill that raster for you, the min slope is: "
           << this_float_map["min_slope_for_fill"] << endl;
      filled_topography = topography_raster.fill(this_float_map["min_slope_for_fill"]);
    }
  
    if (this_bool_map["print_fill_raster"])
    {
      cout << "Let me print the fill raster for you."  << endl;
      string filled_raster_name = OUT_DIR+OUT_ID+"_Fill";
      filled_topography.write_raster(filled_raster_name,raster_ext);
    }
    //==========================================================================


    cout << "\t Flow routing. Note this is memory intensive. If your DEM is very large you may get a segmentation fault here..." << endl;
    // get a flow info object
    LSDFlowInfo FlowInfo(boundary_conditions,filled_topography);
    cout << "Finished flow routing." << endl;

    //=================================================================
    // Now, if you want, calculate drainage areas
    //=================================================================
    if (this_bool_map["print_dinf_drainage_area_raster"])
    {
      cout << "I am writing dinf drainage area to raster." << endl;
      string DA_raster_name = OUT_DIR+OUT_ID+"_dinf_area";
      LSDRaster DA1 = filled_topography.D_inf();
      LSDRaster DA2 = DA1.D_inf_ConvertFlowToArea();
      DA2.write_raster(DA_raster_name,raster_ext);
    }
  
    if (this_bool_map["print_d8_drainage_area_raster"])
    {
      cout << "I am writing d8 drainage area to raster." << endl;
      string DA_raster_name = OUT_DIR+OUT_ID+"_d8_area";
      LSDRaster DA2 = FlowInfo.write_DrainageArea_to_LSDRaster();
      DA2.write_raster(DA_raster_name,raster_ext);
    }
    
    if (this_bool_map["print_QuinnMD_drainage_area_raster"])
    {
      cout << "I am writing Quinn drainage area to raster." << endl;
      string DA_raster_name = OUT_DIR+OUT_ID+"_QMD_area";
      LSDRaster DA3 = filled_topography.QuinnMDFlow();
      DA3.write_raster(DA_raster_name,raster_ext);
    }
    
    if (this_bool_map["print_FreemanMD_drainage_area_raster"])
    {
      cout << "I am writing Freeman drainage area to raster." << endl;
      string DA_raster_name = OUT_DIR+OUT_ID+"_FMD_area";
      LSDRaster DA4 = filled_topography.FreemanMDFlow();
      DA4.write_raster(DA_raster_name,raster_ext);
    }
    if (this_bool_map["print_MD_drainage_area_raster"])
    {
      cout << "I am writing mulitdirection drainage area to raster." << endl;
      string DA_raster_name = OUT_DIR+OUT_ID+"_MD_area";
      LSDRaster DA5 = filled_topography.M2DFlow();
      DA5.write_raster(DA_raster_name,raster_ext);
    }

    // Get the distance from outet raster if you want it.
    if(this_bool_map["print_distance_from_outlet"])
    {
      cout << "I am writing a distant from outlet raster." << endl;
      string FD_raster_name = OUT_DIR+OUT_ID+"_FDIST";
      LSDRaster FD = FlowInfo.distance_from_outlet();
      FD.write_raster(FD_raster_name,raster_ext);
    }

    // This is the logic for a simple stream network
    if (this_bool_map["print_channels_to_csv"]
        || this_bool_map["print_junctions_to_csv"]
        || this_bool_map["print_sources_to_csv"] )
    {
      // calculate the flow accumulation
      cout << "\t Calculating flow accumulation (in pixels)..." << endl;
      LSDIndexRaster FlowAcc = FlowInfo.write_NContributingNodes_to_LSDIndexRaster();

      // load the sources
      cout << "\t Loading Sources, if you have them..." << endl;
      cout << "\t Source file is... " << CHeads_file << endl;
      vector<int> sources;
      if (CHeads_file == "NULL" || CHeads_file == "Null" || CHeads_file == "null")
      {
        cout << endl << endl << endl << "==================================" << endl;
        cout << "The channel head file is null. " << endl;
        cout << "Getting sources from a threshold of "<< this_int_map["threshold_contributing_pixels"] << " pixels." <<endl;
        sources = FlowInfo.get_sources_index_threshold(FlowAcc, this_int_map["threshold_contributing_pixels"]);
    
        cout << "The number of sources is: " << sources.size() << endl;
      }
      else
      {
        cout << "Loading channel heads from the file: " << DATA_DIR+CHeads_file << endl;
        sources = FlowInfo.Ingest_Channel_Heads((DATA_DIR+CHeads_file), "csv",2);
        cout << "\t Got sources!" << endl;
      }

      // now get the junction network
      LSDJunctionNetwork JunctionNetwork(sources, FlowInfo);

      // Print channels and junctions if you want them.
      if( this_bool_map["print_channels_to_csv"])
      {
        cout << "I am going to print the channel network." << endl;
        string channel_csv_name = OUT_DIR+OUT_ID+"_CN";
        JunctionNetwork.PrintChannelNetworkToCSV(FlowInfo, channel_csv_name);
    
        // convert to geojson if that is what the user wants
        // It is read more easily by GIS software but has bigger file size
        if ( this_bool_map["convert_csv_to_geojson"])
        {
          string gjson_name = OUT_DIR+OUT_ID+"_CN.geojson";
          LSDSpatialCSVReader thiscsv(OUT_DIR+OUT_ID+"_CN.csv");
          thiscsv.print_data_to_geojson(gjson_name);
        }
      }
    
      // print junctions
      if( this_bool_map["print_junctions_to_csv"])
      {
        cout << "I am writing the junctions to csv." << endl;
        string channel_csv_name = OUT_DIR+OUT_ID+"_JN.csv";
        JunctionNetwork.print_junctions_to_csv(FlowInfo, channel_csv_name);
    
        if ( this_bool_map["convert_csv_to_geojson"])
        {
          string gjson_name = OUT_DIR+OUT_ID+"_JN.geojson";
          LSDSpatialCSVReader thiscsv(channel_csv_name);
          thiscsv.print_data_to_geojson(gjson_name);
        }
      }
    
      // Print sources
      if( this_bool_map["print_sources_to_csv"])
      {
        string sources_csv_name = OUT_DIR+OUT_ID+"_ATsources.csv";
    
        //write channel_heads to a csv file
        FlowInfo.print_vector_of_nodeindices_to_csv_file_with_latlong(sources, sources_csv_name);
        string sources_csv_name_2 = OUT_DIR+OUT_ID+"_ATsources_rowcol.csv";
        FlowInfo.print_vector_of_nodeindices_to_csv_file(sources, sources_csv_name_2);
    
        if ( this_bool_map["convert_csv_to_geojson"])
        {
          string gjson_name = OUT_DIR+OUT_ID+"_ATsources.geojson";
          LSDSpatialCSVReader thiscsv(sources_csv_name);
          thiscsv.print_data_to_geojson(gjson_name);
        }
      }   // End print sources logic
      
    }     // end logic for tasks related to channel network extraction
  }       // end logic for tasks requiring flow info and filling
}