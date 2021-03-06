#include <iostream>
#include <sstream>
#include <cstdlib>
#include <sys/stat.h>
#include <python2.7/Python.h>
#include "../LSDRasterModel.hpp"

using namespace std;

bool file_check(string name)
{
	struct stat buffer;
	return (stat (name.c_str(), &buffer) == 0);
}

int main(int argc, char *argv[])
{
	//LSDRasterModel mod(0,0);
	LSDRasterModel mod;
	stringstream ss;
	string pathname;
	
	if (argc <= 1)
	{
	  cout << "ERROR: you need to enter a data folder and, \n" 
         << "optionally, a parameter name and a run name" << endl;
    exit(0);
  }
  if (argc > 1)
  {
    pathname = argv[1];
    string lchar = pathname.substr(pathname.length()-2,1);
    string slash = "/";
    cout << "lchar is " << lchar << " and slash is " << slash << endl;
      
    if (lchar != slash)
    {
      cout << "You forgot the frontslash at the end of the path. Appending." << endl;  
      pathname = pathname+slash;
    } 
    cout << "The pathname is: " << pathname << endl;
  }  
  if (argc >2 )
  {
    string param_name = argv[2];
    cout << "The parameter filename is: " << param_name << endl;
    string full_param_name = pathname+param_name;
    cout << "The full path is: " << full_param_name << endl;
    mod.initialize_model(full_param_name);
    
    // add the path to the default filenames
		mod.add_path_to_names( pathname);
  }
	else
	{
		cout << "\n###################################################" << endl;
		cout << "No parameter file supplied" << endl;
		cout << "Creating a template parameter file (template_param)" << endl;
		cout << "###################################################" << endl;
    // first change the default dimensions
    int newrows = 150;
    int newcols = 300;
    mod.resize_and_reset(newrows,newcols);

    // set the end time, print interval, etc
    mod.set_K(0.0001);
    mod.set_endTime(50000);  
    mod.set_print_interval(25);

		string template_param = "template_param.param";
		string full_template_name = pathname+template_param;		
		mod.make_template_param_file(full_template_name);
		
		// add the path to the default filenames
		mod.add_path_to_names( pathname);
		
		// add random asperities to the surface of default model
		mod.random_surface_noise();
	}

	if (argc > 3)
	{
	  string run_name = argv[3];
	  string full_run_name = pathname+run_name;
		// parameter two, run name
		mod.set_name(full_run_name);
	}
	else if (argc == 3)
	{
		if (file_check(mod.get_name() + "_report"))
		{
			string ans;
			cerr << "A run with the name '" << mod.get_name() << "' already exsist, do you wish to overwrite it? (y/n) ";
			cin >> ans;
			if (ans != "y") 
      {
        cout << "You will need to choose another run name, exiting" << endl;
        exit(0);
      }
			else
				cout << "\nOverwriting" << endl;
		}
		ss << "rm " << mod.get_name() << "[0-9]*.asc";
		cout << ss.str() << endl;
		system(ss.str().c_str());
		ss.str("");
		ss << "rm " << mod.get_name() << "*_sa";
		cout << ss.str() << endl;
		system(ss.str().c_str());
	}

	mod.print_parameters();

  // run the model to steady state. 
  // first change the default dimensions
  //int newrows = 300;
  //int newcols = 150;
  //mod.resize_and_reset(newrows,newcols);

  // and turn the hillslopes off, just to save some time
  mod.set_hillslope(false);
  
  // now run to steady state
	mod.reach_steady_state();
	
	// and just to make sure, run it a bit longer
	mod.run_components_combined();
	
	// now print the slope area data
	int area_flag = 1;       // use d8 (like current implementation of FASTSCAPE)
	int slope_flag = 1;      // use d8 (like FASTSCAPE does)
	string SA_ext = "_SA.slopearea";
	string SA_fname = mod.get_name()+SA_ext;
  
  mod.slope_area_data(SA_fname,slope_flag,area_flag);
	
	return 0;
	
}
