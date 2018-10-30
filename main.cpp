// Windows includes (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <math.h>
#include <vector> // STL dynamic memory.

// OpenGL includes
#include <GL/glew.h>
#include <GL/freeglut.h>

// Assimp includes
#include <assimp/cimport.h> // scene importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations

// Project includes
#include "maths_funcs.h"



/*----------------------------------------------------------------------------
MESH TO LOAD
----------------------------------------------------------------------------*/
// this mesh is a dae file format but you should be able to use any other format too, obj is typically what is used
// put the mesh in your project directory, or provide a filepath for it here
#define MESH_NAME_ONE "square.dae"
#define MESH_NAME_TWO "sphere.dae"
#define MESH_NAME_THREE "cylinder.dae"
#define MESH_NAME_FOUR "cone1.dae"
#define MESH_NAME_FIVE "torus.dae"
/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/
GLuint VAO[5];

#pragma region SimpleTypes
typedef struct
{
	size_t mPointCount = 0;
	std::vector<vec3> mVertices;
	std::vector<vec3> mNormals;
	std::vector<vec2> mTextureCoords;
} ModelData;
#pragma endregion SimpleTypes

using namespace std;
GLuint shaderProgramID;

ModelData mesh_data0;
ModelData mesh_data1;
ModelData mesh_data2;
ModelData mesh_data3;
ModelData mesh_data4;

unsigned int mesh_vao = 0;
int width = 800;
int height = 600;

GLuint loc1, loc2, loc3;
GLfloat rotate_x = 0.0f;
GLfloat rotate_y = 0.0f;
GLfloat rotate_z = 0.0f;

GLfloat rootTransx= 0.0f;
GLfloat rootTransy = 0.0f;
GLfloat rootTransz = 0.0f;
GLfloat rootRotx = 0.0f;
GLfloat rootRoty = 0.0f;
GLfloat rootRotz = 0.0f;
GLfloat rotatefinger = 0.0f;
GLfloat rotatewrist = 0.0f;

#pragma region MESH LOADING
/*----------------------------------------------------------------------------
MESH LOADING FUNCTION
----------------------------------------------------------------------------*/

ModelData load_mesh(const char* file_name) {
	ModelData modelData;

	/* Use assimp to read the model file, forcing it to be read as    */
	/* triangles. The second flag (aiProcess_PreTransformVertices) is */
	/* relevant if there are multiple meshes in the model file that   */
	/* are offset from the origin. This is pre-transform them so      */
	/* they're in the right position.                                 */
	const aiScene* scene = aiImportFile(
		file_name, 
		aiProcess_Triangulate | aiProcess_PreTransformVertices
	); 

	if (!scene) {
		fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
		return modelData;
	}

	printf("  %i materials\n", scene->mNumMaterials);
	printf("  %i meshes\n", scene->mNumMeshes);
	printf("  %i textures\n", scene->mNumTextures);

	for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
		const aiMesh* mesh = scene->mMeshes[m_i];
		printf("    %i vertices in mesh\n", mesh->mNumVertices);
		modelData.mPointCount += mesh->mNumVertices;
		for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
			if (mesh->HasPositions()) {
				const aiVector3D* vp = &(mesh->mVertices[v_i]);
				modelData.mVertices.push_back(vec3(vp->x, vp->y, vp->z));
			}
			if (mesh->HasNormals()) {
				const aiVector3D* vn = &(mesh->mNormals[v_i]);
				modelData.mNormals.push_back(vec3(vn->x, vn->y, vn->z));
			}
			if (mesh->HasTextureCoords(0)) {
				const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
				modelData.mTextureCoords.push_back(vec2(vt->x, vt->y));
			}
			if (mesh->HasTangentsAndBitangents()) {
				/* You can extract tangents and bitangents here              */
				/* Note that you might need to make Assimp generate this     */
				/* data for you. Take a look at the flags that aiImportFile  */
				/* can take.                                                 */
			}
		}
	}

	aiReleaseImport(scene);
	return modelData;
}

#pragma endregion MESH LOADING

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS
char* readShaderSource(const char* shaderFile) {
	FILE* fp;
	fopen_s(&fp, shaderFile, "rb");

	if (fp == NULL) { return NULL; }

	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);

	fseek(fp, 0L, SEEK_SET);
	char* buf = new char[size + 1];
	fread(buf, 1, size, fp);
	buf[size] = '\0';

	fclose(fp);

	return buf;
}


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		std::cerr << "Error creating shader..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	const char* pShaderSource = readShaderSource(pShaderText);

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
	glCompileShader(ShaderObj);
	GLint success;
	// check for shader related errors using glGetShaderiv
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024] = { '\0' };
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		std::cerr << "Error compiling "
			<< (ShaderType == GL_VERTEX_SHADER ? "vertex" : "fragment")
			<< " shader program: " << InfoLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	// Attach the compiled shader object to the program object
	glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders()
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	shaderProgramID = glCreateProgram();
	if (shaderProgramID == 0) {
		std::cerr << "Error creating shader program..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(shaderProgramID, "simpleVertexShader.txt", GL_VERTEX_SHADER);
	AddShader(shaderProgramID, "simpleFragmentShader.txt", GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { '\0' };
	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Error linking shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Invalid shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
	glUseProgram(shaderProgramID);
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS
void generateObjectBufferMesh(ModelData mesh_data) {
	/*----------------------------------------------------------------------------
	LOAD MESH HERE AND COPY INTO BUFFERS
	----------------------------------------------------------------------------*/

	//Note: you may get an error "vector subscript out of range" if you are using this code for a mesh that doesnt have positions and normals
	//Might be an idea to do a check for that before generating and binding the buffer.

	
	unsigned int vp_vbo = 0;
	loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
	loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");
	loc3 = glGetAttribLocation(shaderProgramID, "vertex_texture");

	glGenBuffers(1, &vp_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
	glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec3), &mesh_data.mVertices[0], GL_STATIC_DRAW);
	unsigned int vn_vbo = 0;
	glGenBuffers(1, &vn_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
	glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec3), &mesh_data.mNormals[0], GL_STATIC_DRAW);

	//	This is for texture coordinates which you don't currently need, so I have commented it out
	//	unsigned int vt_vbo = 0;
	//	glGenBuffers (1, &vt_vbo);
	//	glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
	//	glBufferData (GL_ARRAY_BUFFER, monkey_head_data.mTextureCoords * sizeof (vec2), &monkey_head_data.mTextureCoords[0], GL_STATIC_DRAW);
	

	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(loc2);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
	glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	
	//	This is for texture coordinates which you don't currently need, so I have commented it out
	//	glEnableVertexAttribArray (loc3);
	//	glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
	//	glVertexAttribPointer (loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
}
#pragma endregion VBO_FUNCTIONS


void display() {

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shaderProgramID);


	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation(shaderProgramID, "model");
	int view_mat_location = glGetUniformLocation(shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation(shaderProgramID, "proj");

	glBindVertexArray(VAO[0]);
	// Root of the Hierarchy
	mat4 view = identity_mat4();
	mat4 persp_proj = perspective(45.0f, (float)width / (float)height, 0.1f, 1000.0f);
	
	
	mat4 root = identity_mat4();
	view = translate(view, vec3(0.0+rootTransx, 0.0+rootTransy, -50.0f+rootTransz));
	root = rotate_x_deg(root, rootRotx);
	root = rotate_y_deg(root, rootRoty);
	root = rotate_z_deg(root, rootRotz);

	// update uniforms & draw
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, root.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data0.mPointCount);
	

	// Set up the child matrix
	// Draw first arm
	mat4 armChild1 = identity_mat4();
	armChild1 = translate(armChild1, vec3(0.0f, 3.0f, 0.0f));
	armChild1 = root * armChild1;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, armChild1.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data0.mPointCount);

	mat4 armChild2 = identity_mat4();
	armChild2 = translate(armChild2, vec3(0.0f, 3.0f, 0.0f));
	armChild2 = armChild1 * armChild2;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, armChild2.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data0.mPointCount);

	mat4 armChild3 = identity_mat4();
	armChild3 = translate(armChild3, vec3(0.0f, 3.0f, 0.0f));
	armChild3 = rotate_z_deg(armChild3, rotate_z);
	armChild3 = armChild2 * armChild3;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, armChild3.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data0.mPointCount);

	mat4 armChild4 = identity_mat4();
	armChild4 = translate(armChild4, vec3(0.0f, 3.0f, 0.0f));
	armChild4 = armChild3 * armChild4;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, armChild4.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data0.mPointCount);

	glBindVertexArray(VAO[4]);
	mat4 torus = identity_mat4();
	torus = translate(torus, vec3(0.0f, 2.0f, 0.0f));
	torus = armChild4 * torus;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, torus.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data4.mPointCount);

	glBindVertexArray(VAO[1]);
	mat4 wrist = identity_mat4();
	wrist = translate(wrist, vec3(0.0f, 0.0f, 0.0f));
	//wrist = rotate_y_deg(wrist, rotate_y);
	wrist = rotate_x_deg(wrist, rotatewrist);
	wrist = translate(wrist, vec3(0.0f, 1.0f, 0.0f));
	wrist = torus * wrist;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, wrist.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data1.mPointCount);

	glBindVertexArray(VAO[2]);
	mat4 finger1 = identity_mat4();
	finger1 = translate(finger1, vec3(0.0f, 1.0f, 0.0f));
	finger1 = rotate_x_deg(finger1, rotatefinger);

	finger1 = wrist * finger1;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, finger1.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data2.mPointCount);

	mat4 finger2 = identity_mat4();
	finger2 = translate(finger2, vec3(-0.5f, 1.0f, 0.0f));
	finger2 = wrist * finger2;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, finger2.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data2.mPointCount);

	mat4 finger3 = identity_mat4();
	finger3 = translate(finger3, vec3(0.5f, 1.0f, 0.0f));
	finger3 = wrist * finger3;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, finger3.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data2.mPointCount);

	glBindVertexArray(VAO[3]);
	mat4 fingertip1 = identity_mat4();
	fingertip1 = translate(fingertip1, vec3(0.0f, 2.0f, 0.0f));
	fingertip1 = finger1 * fingertip1;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, fingertip1.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data3.mPointCount);

	mat4 fingertip2 = identity_mat4();
	fingertip2 = translate(fingertip2, vec3(0.0f, 2.0f, 0.0f));
	fingertip2 = finger2 * fingertip2;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, fingertip2.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data3.mPointCount);

	mat4 fingertip3 = identity_mat4();
	fingertip3 = translate(fingertip3, vec3(0.0f, 2.0f, 0.0f));
	fingertip3 = finger3 * fingertip3;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, fingertip3.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data3.mPointCount);

	glutSwapBuffers();
}


void updateScene() {

	static DWORD last_time = 0;
	DWORD curr_time = timeGetTime();
	if (last_time == 0)
		last_time = curr_time;
	float delta = (curr_time - last_time) * 0.001f;
	last_time = curr_time;

	// Rotate the model 
	rotate_z += 20.0f * delta;
	rotate_z = fmodf(rotate_z, 90.0f);

	rotate_y += 20.0f * delta;
	rotate_y = fmodf(rotate_y, 30.0f);

	rotate_x += 70.0f * delta;
	rotate_x = fmodf(rotate_x, 60.0f);

	// Draw the next frame
	glutPostRedisplay();
}


void init()
{



	// Set up the shaders
	GLuint shaderProgramID = CompileShaders();
	// load mesh into a vertex buffer array
	glGenVertexArrays(5, VAO);

	glBindVertexArray(VAO[0]);
	mesh_data0 = load_mesh(MESH_NAME_ONE);
	generateObjectBufferMesh(mesh_data0);

	glBindVertexArray(VAO[1]);
	mesh_data1 = load_mesh(MESH_NAME_TWO);
	generateObjectBufferMesh(mesh_data1);

	glBindVertexArray(VAO[2]);
	mesh_data2 = load_mesh(MESH_NAME_THREE);
	generateObjectBufferMesh(mesh_data2);

	glBindVertexArray(VAO[3]);
	mesh_data3 = load_mesh(MESH_NAME_FOUR);
	generateObjectBufferMesh(mesh_data3);

	glBindVertexArray(VAO[4]);
	mesh_data4 = load_mesh(MESH_NAME_FIVE);
	generateObjectBufferMesh(mesh_data4);


}

// Placeholder code for the keypress
void keypress(unsigned char key, int x, int y) {
	if (key == 'd') {
		//Translate the base, etc.
		rootTransx += 1;
	}
	if (key == 'a') {
		//Translate the base, etc.
		rootTransx -= 1;
	}
	if (key == 'w') {
		//Translate the base, etc.
		rootTransy += 1;
	}
	if (key == 's') {
		//Translate the base, etc.
		rootTransy -= 1;
	}
	if (key == 'e') {
		//Translate the base, etc.
		rootTransz += 1;
	}
	if (key == 'q') {
		//Translate the base, etc.
		rootTransz -= 1;
	}

	if (key == ']') {
		//rotate the base, etc.
		rootRoty += 2;
	}
	if (key == '[') {
		//rotate the base, etc.
		rootRoty -= 2;
	}

	if (key == 'p') {
		//rotate the base, etc.
		rotatefinger += 2;
	}

	if (key == 'o') {
		//rotate the base, etc.
		rotatefinger -= 2;
	}

	if (key == 'b') {
		//rotate the base, etc.
		rotatewrist += 5;
	}

	if (key == 'v') {
		//rotate the base, etc.
		rotatewrist -= 5;
	}
}

int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutCreateWindow("Hello HAND");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutKeyboardFunc(keypress);

	// A call to glewInit() must be done after glut is initialized!
	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
	return 0;
}
