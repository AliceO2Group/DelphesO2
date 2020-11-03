# Running your simulation and creating your O2 tables
In order to run the simulation and create your own tables you need:
- Configuration file e.g. `default_configfile.ini` where you specify the running configuration
- The LUTs for your detector configuration (specified in the configuration file)
That's it, you are now able to run your simulation and get your tables

As an example with 1000 events in 10 runs split across 10 jobs:
`./createO2tables.py default_configfile.ini --entry CCBAR --nevents 1000 --nruns 10 --njobs 10 -t` 

Simple QA tasks can be run on tables:
`./doanalysis.py 0`

