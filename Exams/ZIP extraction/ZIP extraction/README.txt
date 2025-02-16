**Nexus**
=========
***Nexus is a c++/javascript library for creation and visualization of a batched multiresolution mesh structure***  

[NEXUS](http://vcg.isti.cnr.it/nexus) by [Visual Computing Laboratory](http://vcg.isti.cnr.it) - ISTI - CNR

Contact Us @ federico.ponchio@isti.cnr.it

18 July 2016

#### GENERAL INFO
-----------------

This is a basic set of executables to generate a multi-resolution NEXUS models starting from PLY models. Example PLY files are provided in order to test it.  

These set of executables only run on Windows OS.  

If you need technical assistance about NEXUS, visit the project [website](http://vcg.isti.cnr.it/nexus).  

NEXUS software is released under the GPL license.

#### NEXUS GENERATION
---------------------

In order to generate the NEXUS model:
- Copy your input PLY model in this folder;
- Open the "Build_Nexus.bat" file with a text editor, and substitute "gargo.ply" with the name of your input PLY file, and "gargo.nxs" with the name of the output that you want to generate;
- Save and launch "Build_Nexus.bat". At the end, a .nxs file should be saved in this folder;

Alternatively you can generate the NEXUS model just dragging and dropping your input PLY model icon on the "nxsbuild.exe" icon 
(however this direct generation mode does not allow you to modify the build options, please open the command line prompt in this folder and type "nxsbuild.exe --help" to know more about all the build options avalaible).  

NEXUS handles both triangular meshes and point clouds. Color is encoded as color-per-vertex or as single texture. 


#### NEXUS COMPRESSION
----------------------

If you want to generate a compressed version of the model:
- Copy your input uncompressed NEXUS model in this folder;
- Open the "Compress_Nexus.bat" file with a text editor, and substitute "gargo.nxs" with the name of your uncompressed NEXUS file, and "gargo_c.nxs" with the name of the output that you want to generate;
- Save and launch "Compress_Nexus.bat". At the end, a compressed .nxs file should be saved in this folder;

(Please open the command line prompt in this folder and type "nxsedit.exe --help" to know more about all the edit options avalaible). 


#### NEXUS VISUALIZATION
------------------------

To locally visualize the generated NEXUS model just drag and drop the relative file icon on the "nxsview.exe" icon.