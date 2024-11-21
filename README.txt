House Rendering Program

Brief Description:
This program renders the interior of a house using OpenGL. 
It provides a virtual camera that allows the user to navigate through the scene. 
The house is composed of various textured objects such as walls, floors, furniture, and decorations.

Instructions for Compiling: (Windows System)
- Launch CodeBlocks with all the libraries installed (this include glfw, glew, glm, opengl, glut)
- Open Setting > Compiler, and set the linker to -lfreeglut -lglfw3 -lglew32 -lopengl32
- Create a new project: go to File > New > Project
- On the window pops up, click on Console Application, then click Go
- Continue by clicking on Next and enter a project title, for example, type in "A4_3388", then click on Next and Finish
- In the source folder, copy and replace the main.cpp file with the cpp file I have submitted.
- Open the folder where the project is located, insert all the ply and bmp files into the folder(a total of 20 files). They should be in the same layer with main.cpp
- Then go back to Codeblocks, right click on the project title, click on Add Files, select all the ply and bmp files in the folder, then click on OK.
- Now, in the project, you can see there's a folder called Others, and it should now include all the 20 files.
- Click on Build and Run, and the program should successfully run


Camera Setup for Screenshots:
Please find the submitted House1.png and House2.png, both of which capture the entirety of my desktop screen.
House1.png: This image was captured by holding down the backward key followed by the left key for a few seconds.
House2.png: : This image was taken by pressing and holding the forward key, followed by the right key, and then the backward key, each for several seconds.
To capture the screenshots of the house interior, the camera was positioned and oriented to provide a comprehensive view of the scene. 
The camera's position and target were adjusted to focus on the central area of the room, ensuring that more objects are visible in the screenshots.
