# brats_mha_to_tvflow
Converts 3d mha files to nrrd and png and runs tvflow on them


Lab Visualisation & Medical Image Analysis SS2019
Institute of Computer Science II

Author: Christian Breiderhoff
created on June 2019

Written in python 3.5

Necessary packages
  - numpy
  - pypng
  - pynrrd
  - SimpleITK
 
 The program performs basically 4 steps:
  - read mha file with with SimpleITK
  - save every slice as nrrd and png file (8bit gray scale)
  - run tvflow on every slice with nrrd files (output is also a nrrd file)
  - convert tvflow nrrd file to png image (8bit gray scale)
 
            
  You can test the functionality with by running "python3 test.py". It performs the above steps with "test_in_out/test.mha" and outputs in the same directory.
 
  If you want to run on the whole Brats2015 data set you have to provide it in the following directories:
 
    - brats_mha_to_tvflow/../Dataset/BRATS2015_Training/ ...
    - brats_mha_to_tvflow/../Dataset/BRATS2015_Testing/ ...

   To change directories please have a look at "config.py" 
   The output will be put in the "brats_mha_to_tvflow/../Dataset" dirctory. Run it by running "python3 mha_to_tvflow.py"
  
   You can configure you tvflow parameters at the top of "config.py".
  
   The program performs the task in a multi threaded style to increase performance. It runs in number_of_threads=number_of_cpu_cores per default.
   You can configure the number of threads at the top of "mha_to_tvflow.py".
