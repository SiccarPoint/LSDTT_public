//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// chi_map_sensitivity_analysis
//
// This program takes two arguments, the path name and the driver name
// It then runs the chi analysis for the parameter space defined in the driver
// file, allowing the user to examine the effects of changing m/n value,
// number of target nodes, minimum segment length, sigma and target skip value.
// At present it just spits out an output file for each iteration.
// Now also kicks out a table with the parameters and best fit m/n.
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// 18/03/2014
// David Milodowski
// University of Edinburgh
//
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "../LSDStatsTools.hpp"
#include "../LSDChiNetwork.hpp"
#include "../LSDRaster.hpp"
#include "../LSDIndexRaster.hpp"
#include "../LSDFlowInfo.hpp"
#include "../LSDJunctionNetwork.hpp"
#include "../LSDIndexChannelTree.hpp"
#include "../LSDBasin.hpp"
#include "../LSDShapeTools.hpp"


int main (int nNumberofArgs,char *argv[])
{
  //Test for correct input arguments
  if (nNumberofArgs!=3)
  {
    cout << "FATAL ERROR: wrong number inputs. The program needs the path name, the driver file name" << endl;
    exit(EXIT_SUCCESS);
  }

  string path_name = argv[1];
  string f_name = argv[2];

  cout << "The path is: " << path_name << " and the filename is: " << f_name << endl;

  string full_name = path_name+f_name;

  ifstream file_info_in;
  file_info_in.open(full_name.c_str());
  if( file_info_in.fail() )
  {
    cout << "\nFATAL ERROR: The parameter \"" << full_name
         << "\" doesn't exist" << endl;
    exit(EXIT_FAILURE);
  }

  string DEM_ID,raster_ext;
  string dem_ext = "_dem";
  string fill_ext = "_fill";
  string acc_ext = "_ACC";
  string txt_extension = ".txt";

  // initialise variables to be assigned from .driver file
  string DATA_DIR, OUTPUT_DIR, CHeads_file;
  int threshold;
  float minimum_stream_order;
  float A_0;
  float movern;
  float Minimum_Slope;
  int n_iterations;
  string temp;
  int minimum_segment_length;
  float sigma;
  int target_nodes;
  int skip;
  int threshold_pixels_for_chi;
  bool test_drainage_boundaries = true;

  file_info_in >> temp >> DATA_DIR
               >> temp >> OUTPUT_DIR
               >> temp >> DEM_ID
               >> temp >> CHeads_file
               >> temp >> raster_ext
               >> temp >> minimum_stream_order
               >> temp >> Minimum_Slope
               >> temp >> A_0
               >> temp >> movern
               >> temp >> n_iterations
               >> temp >> target_nodes
               >> temp >> minimum_segment_length
               >> temp >> sigma
               >> temp >> skip
               >> temp >> threshold_pixels_for_chi
               >> temp >> test_drainage_boundaries;

  file_info_in.close();

  cout << "PARAMETERS FOR Chi mapping\n\t DEM_ID = " << DEM_ID
               << "\n\t DATA_DIR " << DATA_DIR
               << "\n\t OUTPUT_DIR " << OUTPUT_DIR
               << "\n\t CHeads_file " << CHeads_file
               << "\n\t raster_ext " << raster_ext
               << "\n\t Basin Stream Order " << minimum_stream_order
               << "\n\t Minimum Slope (for fill function) " << Minimum_Slope
               << "\n\t A_0 " <<  A_0
               << "\n\t m/n: " <<  movern
               << "\n\t number of MC iterations " << n_iterations
               << "\n\t min segment length: " <<  minimum_segment_length
               << "\n\t target nodes: " <<  target_nodes
               << "\n\t sigma: " <<  sigma
               << "\n\t Skip: " <<  skip 
               << "\n\t threshold_pixels_for_chi: " << threshold_pixels_for_chi 
               << "\n\t test_drainage_boundaries: " << test_drainage_boundaries 
               << endl << endl;

  // Additional parameters for chi analysis
  int organization_switch = 1;
  int pruning_switch = 3;
  float pruning_threshold;// = target_stream_order - 1;
  
  cout << "Setting boundary conditions" << endl;

  // set no flux boundary conditions
  vector<string> boundary_conditions(4);
  boundary_conditions[0] = "No";
  boundary_conditions[1] = "no flux";
  boundary_conditions[2] = "no flux";
  boundary_conditions[3] = "No flux";
  
  // check the DEM
  cout << "I am checking to make sure the DEM exists." << endl;

  // load the  DEM
  LSDRaster topography_raster((DATA_DIR+DEM_ID), raster_ext);
  cout << "Got the dem: " <<  DATA_DIR+DEM_ID << endl;
  
  
  int NRows = topography_raster.get_NRows();
  int NCols = topography_raster.get_NCols();
  float NoData = topography_raster.get_NoDataValue();
  float XMin =topography_raster.get_XMinimum();
  float YMin = topography_raster.get_YMinimum();
  float Resolution = topography_raster.get_DataResolution();
  map<string,string> GRS = topography_raster.get_GeoReferencingStrings();

  cout << "Filling topography." << endl;
  LSDRaster filled_topography = topography_raster.fill(Minimum_Slope);
  
  string filled_raster_name = DATA_DIR+DEM_ID+"_Fill";
  
  //filled_topography.write_raster(filled_raster_name,raster_ext);
  
  cout << "\t Flow routing..." << endl;
  // get a flow info object
  LSDFlowInfo FlowInfo(boundary_conditions,filled_topography);

  // calcualte the distance from outlet
  LSDRaster DistanceFromOutlet = FlowInfo.distance_from_outlet();

  cout << "\t Loading Sources..." << endl;
  // load the sources
  vector<int> sources;
  if (CHeads_file == "NULL" || CHeads_file == "Null" || CHeads_file != "null")
  {
      cout << endl << endl << endl << "==================================" << endl;
    cout << "The channel head file is null. Getting sources from a deafault threshold of 50 pixels." <<endl;
    cout << "If you want to change the threshold you need to go into map_chi_gradient.cpp," << endl;
    cout << "and change it. Search for LeighGriffiths in the source code, the threshold is two lines below that." << endl;
    LSDIndexRaster FlowAcc = FlowInfo.write_NContributingNodes_to_LSDIndexRaster();
    sources = FlowInfo.get_sources_index_threshold(FlowAcc, threshold_pixels_for_chi);
    
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

  string SO_ext = "_SO";
  string JI_ext = "_JI";
  LSDIndexRaster SOArray = JunctionNetwork.StreamOrderArray_to_LSDIndexRaster();
  LSDIndexRaster JIArray = JunctionNetwork.JunctionIndexArray_to_LSDIndexRaster();
  SOArray.write_raster((OUTPUT_DIR+DEM_ID+SO_ext),raster_ext);
  JIArray.write_raster((OUTPUT_DIR+DEM_ID+JI_ext),raster_ext);
  int max_stream_order = JunctionNetwork.get_maximum_stream_order();
  Array2D<float> ChiGradientArray(NRows,NCols,NoData);
  
  cout << "The maximum stream order is: " << max_stream_order << endl;

  // need to get base-level nodes , otherwise these catchments will be missed!
  vector< int > BaseLevelJunctions_Initial = JunctionNetwork.get_BaseLevel_DonorJunctions();
  vector< int > BaseLevelJunctions;
  int N_BaseLevelJuncs = BaseLevelJunctions_Initial.size();
  cout << "The number of base level junctions is: " << N_BaseLevelJuncs << endl; 
  
  
  // remove base level junctions for which catchment is truncated
  if (test_drainage_boundaries)
  {
    cout << endl << endl << "I am going to remove any basins draining to the edge." << endl;
  
    for(int i = 0; i < N_BaseLevelJuncs; ++i)
    {
      //cout << "I'm checking node " << i << " to see if it is truncated." << endl;
      bool keep_base_level_node = true;
      
      // get donor nodes to base level nodes node
      vector<int> DonorJunctions = JunctionNetwork.get_donor_nodes(BaseLevelJunctions_Initial[i]);
      int N_Donors = DonorJunctions.size();
      //cout << "It has " << N_Donors << " donor junctions." << endl;
      
      if (N_Donors == 1)    // this is a tiny base level node with no donors, we won't keep it. 
      {
        //cout << "This is a base level junction! Junction is: " << BaseLevelJunctions_Initial[i] << endl
        //     << "and donor is: " << DonorJunctions[0] << endl;
        keep_base_level_node = false; 
      }
      else
      {
        // check to see if either donor nodes are truncated - basically want to keep
        // basins that flow onto edge of DEM
        
        for(int i_donor = 0; i_donor < N_Donors; ++ i_donor)
        {
          bool IsTruncated = JunctionNetwork.node_tester(FlowInfo,DonorJunctions[i_donor]);
          if(IsTruncated == true)
          {
            keep_base_level_node = false;
          }
        }
      }
      if(keep_base_level_node == true) BaseLevelJunctions.push_back(BaseLevelJunctions_Initial[i]);
    }
    
    cout << "I have removed the channels that are draining from the edge of the DEM." << endl;
    cout << "I now have " << N_BaseLevelJuncs << " base level junctions" << endl;
  }
  else
  {
    BaseLevelJunctions =BaseLevelJunctions_Initial;
  }
  
  // Correct number of base level junctions
  N_BaseLevelJuncs = BaseLevelJunctions.size();

  // calculate chi for the entire DEM
  float threshold_area_for_chi = Resolution*Resolution*float(threshold_pixels_for_chi);
  LSDRaster chi_coordinate = FlowInfo.get_upslope_chi_from_all_baselevel_nodes(movern,A_0,threshold_area_for_chi);
  
  string chi_coord_string = OUTPUT_DIR+DEM_ID+"chi_coord";
  chi_coordinate.write_raster(chi_coord_string,raster_ext);

  string csv_fname = OUTPUT_DIR+DEM_ID+"_FullChi_tree.csv";
  ofstream chitree_out;
  
  // the precision needs to be high for the lat long and UTM coordinates to acutally
  // give a reasonable number of significant digits. 
  chitree_out.precision(12);
  chitree_out.open(csv_fname.c_str());
  chitree_out << "id,x,y,lat,long,chi,mean_chi_gradient" << endl;
  
    // initilise the converter
  LSDCoordinateConverterLLandUTM Converter;
  
  for(int target_stream_order_this_iter = minimum_stream_order; target_stream_order_this_iter<=max_stream_order; ++target_stream_order_this_iter)
  {
    pruning_threshold = target_stream_order_this_iter - 1;

    // Find all junctions for target stream order basins
    vector< int > BasinOutletNodes = JunctionNetwork.extract_basins_order_outlet_junctions(target_stream_order_this_iter, FlowInfo);

    for(int i = 0; i < N_BaseLevelJuncs; ++i)
    {
      int order = JunctionNetwork.get_StreamOrder_of_Junction(BaseLevelJunctions[i]);
      if(order == target_stream_order_this_iter) BasinOutletNodes.push_back(BaseLevelJunctions[i]);
    }

    int N_basins = BasinOutletNodes.size();

    vector<float> MeanChiGradient;
    vector<int> OutletJunctions;
    for(int i_basin = 0; i_basin < N_basins; ++i_basin)
    {
      int outlet_junction = BasinOutletNodes[i_basin];
      OutletJunctions.push_back(outlet_junction);
      string jn_name = itoa(outlet_junction);
      string uscore = "_";
      jn_name = uscore+jn_name;
      // now find longest channel upstream
      cout << "outlet junction " << outlet_junction << ", catchment " << i_basin+1 << " of " << N_basins << ":- creating main stem" << endl;
      LSDIndexChannel main_stem = JunctionNetwork.generate_longest_index_channel_in_basin(outlet_junction, FlowInfo, DistanceFromOutlet);
      cout << "got main stem channel, with " << main_stem.get_n_nodes_in_channel() << " nodes" <<  endl;

      // now get the best fit m over n for all the tributaries
      LSDIndexChannelTree ChannelTree(FlowInfo, JunctionNetwork, outlet_junction, organization_switch, DistanceFromOutlet, pruning_switch, pruning_threshold);

    // print a file that can be ingested bt the chi fitting algorithm
      string Chan_fname = "_ChanNet";
      string Chan_ext = ".chan";
      string Chan_for_chi_ingestion_fname = OUTPUT_DIR+DEM_ID+Chan_fname+jn_name+Chan_ext;
      ChannelTree.print_LSDChannels_for_chi_network_ingestion(FlowInfo, filled_topography, DistanceFromOutlet, Chan_for_chi_ingestion_fname);
      ChannelTree.convert_chan_file_for_ArcMap_ingestion(Chan_for_chi_ingestion_fname);
      // Create chi network automatically here, rather than reading in a file
      // create the chi network
      LSDChiNetwork ChiNetwork(Chan_for_chi_ingestion_fname);
      ChiNetwork.extend_tributaries_to_outlet();

      // Now run the chi-analysis to contruct the best fit m/n value for each
      // scenario, and kick out chi-profiles for each iteration.
      // first get a string with some paramter values; needed for file names
      string sigma_str;
      string skip_str;
      string msl_str;
      string tn_str;
      string param_str;
      sigma_str = static_cast<ostringstream*>( &(ostringstream() << sigma) )->str();
      skip_str = static_cast<ostringstream*>( &(ostringstream() << skip) )->str();
      msl_str = static_cast<ostringstream*>( &(ostringstream() << minimum_segment_length) )->str();
      tn_str = static_cast<ostringstream*>( &(ostringstream() << target_nodes) )->str();
      param_str = uscore+sigma_str+uscore+skip_str+uscore+msl_str+uscore+tn_str;

      string fpt_ext = ".tree";
      // convert the m/n ratio to a string for the output filename
      string prefix_movn = static_cast<ostringstream*>( &(ostringstream() << movern) )->str();
      // get the breaks of all the channels
      ChiNetwork.split_all_channels(A_0, movern, n_iterations, skip, target_nodes, minimum_segment_length, sigma);
      // monte carlo sample all channels
      ChiNetwork.monte_carlo_sample_river_network_for_best_fit_after_breaks(A_0, movern, n_iterations, skip, minimum_segment_length, sigma);
      string fpt_mc = "_fullProfileMC_forced_" + prefix_movn+param_str;
      ChiNetwork.print_channel_details_to_file_full_fitted((OUTPUT_DIR+DEM_ID+fpt_mc+jn_name+fpt_ext));

      // Loop through ChiNetwork object.  Calculate average steepness of each node that has a stream order >= (basin order - 1)
      vector< vector<int> > NetworkNodes = ChiNetwork.get_node_indices();
      vector< vector<float> > NetworkChiGradients = ChiNetwork.get_m_means();

      int N_channels = NetworkNodes.size();
      vector<float> ChannelChiGradient;
      
      for(int i_channel = 0; i_channel < N_channels; ++i_channel)
      {
        int N_nodes = NetworkNodes[i_channel].size();
        int row, col;
        for(int i_node = 0; i_node < N_nodes; ++i_node)
        {
          FlowInfo.retrieve_current_row_and_col(NetworkNodes[i_channel][i_node], row, col);
          if((SOArray.get_data_element(row,col) >= pruning_threshold) && (SOArray.get_data_element(row,col) >= minimum_stream_order))
          {
            ChiGradientArray[row][col]=NetworkChiGradients[i_channel][i_node];
            ChannelChiGradient.push_back(NetworkChiGradients[i_channel][i_node]);
            
            float x_loc, y_loc; 
            double lat,longitude;
            chi_coordinate.get_x_and_y_locations(row, col, x_loc, y_loc);
            chi_coordinate.get_lat_and_long_locations(row, col, lat, longitude, Converter);
            
            // now print to the csv file
            chitree_out << NetworkNodes[i_channel][i_node] << "," <<  x_loc << "," << y_loc << ","
                        << lat << "," << longitude << "," << chi_coordinate.get_data_element(row,col) << "," 
                        << NetworkChiGradients[i_channel][i_node] << endl;

          }
        }
      }
      MeanChiGradient.push_back(get_mean(ChannelChiGradient));

    }
    // Now write output rasters - a raster of the outlet junction number for the catchment, and a raster of the mean chi gradient for that catchment
    Array2D<float> BasinAverageChiGradients(NRows,NCols,NoData);
    Array2D<int> BasinJunctionNumbers(NRows,NCols,NoData);
    for(int i_basin = 0; i_basin < N_basins; ++i_basin)
    {
      LSDBasin Basin(OutletJunctions[i_basin], FlowInfo, JunctionNetwork);
      vector<int> BasinNodes = Basin.get_BasinNodes();
      int row, col;
      int N_nodes_in_basin = BasinNodes.size();
      for(int i = 0; i<N_nodes_in_basin; ++i)
      {
        FlowInfo.retrieve_current_row_and_col(BasinNodes[i], row, col);
        BasinAverageChiGradients[row][col] = MeanChiGradient[i_basin];
        BasinJunctionNumbers[row][col] = BasinOutletNodes[i_basin];
      }
    }
    string stream_order_string = itoa(target_stream_order_this_iter);

    string output_ext_1 = "_" + stream_order_string + "_basin_steepness";
    string output_ext_2 = "_" + stream_order_string + "_basin_ID";

    LSDRaster BasinChiGradients(NRows,NCols,XMin,YMin,Resolution,NoData,BasinAverageChiGradients, GRS);
    LSDIndexRaster BasinJunctions(NRows,NCols,XMin,YMin,Resolution,NoData,BasinJunctionNumbers, GRS);
    BasinChiGradients.write_raster((OUTPUT_DIR+DEM_ID+output_ext_1),raster_ext);
    BasinJunctions.write_raster((OUTPUT_DIR+DEM_ID+output_ext_2),raster_ext);
    
    cout << "Done with stream order: " << target_stream_order_this_iter << endl;
    
  }
  string output_ext_3 = "_chi_gradient";
  LSDRaster ChiGradients(NRows,NCols,XMin,YMin,Resolution,NoData,ChiGradientArray,GRS);
  ChiGradients.write_raster((OUTPUT_DIR+DEM_ID+output_ext_3),raster_ext);
  
  chitree_out.close();

}
