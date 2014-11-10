// ==========================================
//     Project Part 1
// ==========================================
//
//  Team members:
//     Kieran Hannigan
//        21151118
//     Thomas Frazer Eagle Drake-Brockman
//        21150739
//
//  The project was written and tested on
//  GNU/Linux Fedora and Ubuntu. The Windows
//  libraries were removed to keep version
//  control clean, but if Windows is used for
//  marking and libraries are all configured,
//  the makefile should be fairly agnostic.
//
//  Game mode is the individual additional 
//  functionality implemented by Kieran.
// 
//  Thomas' individual contributions are
//  object selection, object duplication,
//  the ability to hide and unhide objects.
// 
//  The additions included in vsync and 
//  full screen modes, along with character
//  jumping, and alpha transparency are 
//  examples of implementation of additional
//  functionality by the team.
//
//  Please also note that access to the private
//  GitHub repository is available on request.
// ==========================================


#include "Angel.h"

#include <stdlib.h>
#include <dirent.h>
#include <time.h>

// Open Asset Importer header files (in ../../assimp--3.0.1270/include)
// This is a standard open source library for loading meshes, see gnatidread.h
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Previous values are saved when fullscreen mode is toggled to facilitate graceful restore.
GLint windowHeight=640, windowWidth=960, prevWindowHeight=640, prevWindowWidth=960;

// gnatidread.cpp is the CITS3003 "Graphics n Animation Tool Interface & Data Reader" code
// This file contains parts of the code that you shouldn't need to modify (but, you can).
#include "gnatidread.h"
#include "gnatidread2.h"

using namespace std;    // Import the C++ standard functions (e.g., min) 


// IDs for the GLSL program and GLSL variables.
GLuint shaderProgram; // The number identifying the GLSL shader program
GLuint vPosition, vNormal, vTexCoord, vBoneIDs, vBoneWeights; // IDs for vshader input vars (from glGetAttribLocation)
GLuint projectionU, viewU, modelViewU, boneTransformsU; // IDs for uniform variables (from glGetUniformLocation)

static float viewDist = 20; // Distance from the camera to the centre of the scene
static float camRotSidewaysDeg=0; // rotates the camera sideways around the centre
static float camRotUpAndOverDeg=10; // rotates the camera up and over the centre.

mat4 projection; // Projection matrix - set in the reshape function
mat4 view; // View matrix - set in the display function.

// These are used to set the window title
char lab[] = "Project 1";
char *programName = NULL; // Set in main 
int numDisplayCalls = 0; // Used to calculate the number of frames per second

// -----Meshes----------------------------------------------------------
// Uses the type aiMesh from ../../assimp--3.0.1270/include/assimp/mesh.h
//                      (numMeshes is defined in gnatidread.h)
// ---------------------------------------------------------------------
aiMesh* meshes[numMeshes]; // For each mesh we have a pointer to the mesh to draw
GLuint vaoIDs[numMeshes]; // and a corresponding VAO ID from glGenVertexArrays
const aiScene* scenes[numMeshes];

// -----Textures---------------------------------------------------------
//                      (numTextures is defined in gnatidread.h)
texture* textures[numTextures]; // An array of texture pointers - see gnatidread.h
GLuint textureIDs[numTextures]; // Stores the IDs returned by glGenTextures


// ------Scene Objects--------------------------------------------------------------------------------------
//
// For each object in a scene we store the following
// Note: the following is exactly what the sample solution uses, you can do things differently if you want.
typedef struct {
    vec4 loc;
    float scale;
    float angles[3]; // rotations around X, Y and Z axes.
    float diffuse, specular, ambient; // Amount of each light component
    float shine;
    vec3 rgb;
    float alpha;
    float brightness; // Multiplies all colours
    int meshId;
    int texId;
    float texScale;
} SceneObject;

const int maxObjects = 1024; // Scenes with more than 1024 objects seem unlikely

SceneObject sceneObjs[maxObjects]; // An array storing the objects currently in the scene.
bool hidden[maxObjects];

int nObjects=0; // How many objects are currenly in the scene.
int currObject=-1; // The current object
int toolObj = -1;  // The object currently being modified

int selectMenuId;

float fov = 20.0;

// ---------------------------------------------
//             Game mode variables
// ---------------------------------------------
// Game mode can be activated by selecting
// the appropriate item from the right click
// menu. It can also be toggled with the 'g' key. 
//
// Game mode includes the solution to the close-up
// task (as does design mode to a degree) by
// changing the field of view of the projection
// matrix. This is present in real-world
// game applications where characters zoom in
// with gun scopes or binoculars.
//
// ================ Controls ===================
//                  w - run forward 
//                  a - strafe left 
//                  s - run back 
//                  d - strafe right 
//              space - jump
//              mouse - pitch and yaw
//                      (camera/head tilt)
//        mouse wheel - dynamic fov change
//                     (binoculars / gun scope)
//                 v* - toggle vsync
//                 f* - toggle fullscreen
//
// * also works in design mode
// 
// The arrow keys also perform head movement
// for machines with no point and click input.
// 
// Page up and down also perform the fov change
// (or zooming in design mode).
// ---------------------------------------------

bool gameMode = false;
bool vsync = true;
bool fullscreen = false;
int Hz = 60; // monitor refresh rate for vsync

// Scaling constants.
// The delta will also be scaled according to time as opposed
// to processing speed to prevent time compression occuring
// on modern systems.
const float turnScale = 0.1;
const float mouseTurnScale = 0.2;
const float moveScale = 0.003;
const float gravity = 0.00004;
const float impulse = 0.01;
// Position and rotation variables.
// Each represents the current variation from 0,0,0,0,0,0.
// Used to construct the view matrix in game mode.
static float yaw = 0.0;
static float pitch = 0.0;
static float dx = 0.0;
static float dy = 1;
static float dz = 0.0;
// Intertia, impulse, and gravity are used in character jumping.
static float inertia = impulse;
// These variables act as toggles to detect key depression.
// The approach is more graceful than allowing key repetition
// (which varies from system to system and device to device)
// to dictate rate of change.
int gameFOV = 65;
bool runForward = false;
bool runBack = false;
bool pitchUp = false;
bool pitchDown = false;
bool strafeLeft = false;
bool strafeRight = false;
bool yawLeft = false;
bool yawRight = false;
bool jump = false;
int t = 0; // set in display: = glutGet(GLUT_ELAPSED_TIME);
int dt = 0;

// ------------------------------------------------------------------------------------------
// Reshape is used in many aspects of the game mode operation. A function signature could be
// declared, but ordering is more consistent with other parts of the program.
// ------------------------------------------------------------------------------------------

void reshape(int width, int height) {
    windowWidth = width;
    windowHeight = height;

    // Restrict FOV to the quake---sniper spectrum.
    if (gameFOV < 10) {
        gameFOV = 10;
    }
    if (gameFOV > 90) {
        gameFOV = 90;
    }

    glViewport(0, 0, width, height);

    fov = 20;

    // Correct for elongated window shapes.
    if (width < height) {
      fov *= (float)height / (float)width;
    }

    // You'll need to modify this so that the view is similar to that in the sample solution.
    // In particular: 
    //   - the view should include "closer" visible objects (slightly tricky)
    //   - when the width is less than the height, the view should adjust so that the same part
    //     of the scene is visible across the width of the window.

    if (gameMode) {
        projection = Perspective(gameFOV, (float)width/(float)height, 0.1, 5.0);
    } else {
        projection = Perspective(fov, (float)width/(float)height, 0.1, 10.0);
    }
}

// --------------------------------------
// Toggle between game modes.
// --------------------------------------

static void switchMode() {
    if (!gameMode) {
        glutSetCursor(GLUT_CURSOR_NONE); 
        gameMode = true;
    } else {
        glutSetCursor(GLUT_CURSOR_INHERIT);
        gameMode = false;
    }
    reshape(windowWidth, windowHeight);
}

// -------------------------------------
// Toggle fullscreen state (gracefully).
// -------------------------------------

static void toggleFullScreen() {
    if(!fullscreen){
        prevWindowWidth = windowWidth;
        prevWindowHeight = windowHeight;
        glutFullScreen();
        fullscreen = true;
    } else if(fullscreen){
        glutReshapeWindow(prevWindowWidth, prevWindowHeight);
        reshape(prevWindowWidth, prevWindowHeight);
        fullscreen = false;
    }
}

// ------------------------------------------------------------
// Loads a texture by number, and binds it for later use.  
// ------------------------------------------------------------

void loadTextureIfNotAlreadyLoaded(int i) {
    if(textures[i] != NULL) return; // The texture is already loaded.

    textures[i] = loadTextureNum(i); CheckError();
    glActiveTexture(GL_TEXTURE0); CheckError();

    // Based on: http://www.opengl.org/wiki/Common_Mistakes
    glBindTexture(GL_TEXTURE_2D, textureIDs[i]);
    CheckError();

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textures[i]->width, textures[i]->height,
                 0, GL_RGB, GL_UNSIGNED_BYTE, textures[i]->rgbData); CheckError();
    glGenerateMipmap(GL_TEXTURE_2D); CheckError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); CheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); CheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); CheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); CheckError();

    glBindTexture(GL_TEXTURE_2D, 0); CheckError(); // Back to default texture
}


//------Mesh loading ----------------------------------------------------
//
// The following uses the Open Asset Importer library via loadMesh in 
// gnatidread.h to load models in .x format, including vertex positions, 
// normals, and texture coordinates.
// You shouldn't need to modify this - it's called from drawMesh below.

void loadMeshIfNotAlreadyLoaded(int meshNumber) {

    if(meshNumber>=numMeshes || meshNumber < 0) {
        printf("Error - no such  model number");
        exit(1);
    }

    if(meshes[meshNumber] != NULL)
        return; // Already loaded

    const aiScene* scene = loadScene(meshNumber);
    scenes[meshNumber] = scene;
    aiMesh* mesh = scene->mMeshes[0];
    meshes[meshNumber] = mesh;

    glBindVertexArray( vaoIDs[meshNumber] );

    // Create and initialize a buffer object for positions and texture coordinates, initially empty.
    // mesh->mTextureCoords[0] has space for up to 3 dimensions, but we only need 2.
    GLuint buffer[1];
    glGenBuffers( 1, buffer );
    glBindBuffer( GL_ARRAY_BUFFER, buffer[0] );
    glBufferData( GL_ARRAY_BUFFER, sizeof(float)*(3+3+3)*mesh->mNumVertices,
                  NULL, GL_STATIC_DRAW );

    int nVerts = mesh->mNumVertices;
    // Next, we load the position and texCoord data in parts.  
    glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(float)*3*nVerts, mesh->mVertices );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(float)*3*nVerts, sizeof(float)*3*nVerts, mesh->mTextureCoords[0] );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(float)*6*nVerts, sizeof(float)*3*nVerts, mesh->mNormals);

    // Load the element index data
    GLuint elements[mesh->mNumFaces*3];
    for(GLuint i=0; i < mesh->mNumFaces; i++) {
        elements[i*3] = mesh->mFaces[i].mIndices[0];
        elements[i*3+1] = mesh->mFaces[i].mIndices[1];
        elements[i*3+2] = mesh->mFaces[i].mIndices[2];
    }

    GLuint elementBufferId[1];
    glGenBuffers(1, elementBufferId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferId[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * mesh->mNumFaces * 3, elements, GL_STATIC_DRAW);

    // vPosition it actually 4D - the conversion sets the fourth dimension (i.e. w) to 1.0         
    glVertexAttribPointer( vPosition, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );
    glEnableVertexAttribArray( vPosition );

    // vTexCoord is actually 2D - the third dimension is ignored (it's always 0.0)
    glVertexAttribPointer( vTexCoord, 3, GL_FLOAT, GL_FALSE, 0,
                           BUFFER_OFFSET(sizeof(float)*3*mesh->mNumVertices) );
    glEnableVertexAttribArray( vTexCoord );
    glVertexAttribPointer( vNormal, 3, GL_FLOAT, GL_FALSE, 0,
                           BUFFER_OFFSET(sizeof(float)*6*mesh->mNumVertices) );
    glEnableVertexAttribArray( vNormal );
    CheckError();

    // Get boneIDs and boneWeights for each vertex from the imported mesh data
    GLint boneIDs[mesh->mNumVertices][4];
    GLfloat boneWeights[mesh->mNumVertices][4];
    getBonesAffectingEachVertex(mesh, boneIDs, boneWeights);

    GLuint buffers[2];
    glGenBuffers( 2, buffers );  // Add two vertex buffer objects
    
    glBindBuffer( GL_ARRAY_BUFFER, buffers[0] ); CheckError();
    glBufferData( GL_ARRAY_BUFFER, sizeof(int)*4*mesh->mNumVertices, boneIDs, GL_STATIC_DRAW ); CheckError();
    glVertexAttribIPointer(vBoneIDs, 4, GL_INT, 0, BUFFER_OFFSET(0)); CheckError();
    glEnableVertexAttribArray(vBoneIDs);     CheckError();
    
    glBindBuffer( GL_ARRAY_BUFFER, buffers[1] );
    glBufferData( GL_ARRAY_BUFFER, sizeof(float)*4*mesh->mNumVertices, boneWeights, GL_STATIC_DRAW );
    glVertexAttribPointer(vBoneWeights, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(vBoneWeights);    CheckError();
}


// --------------------------------------
static void mouseClickOrScroll(int button, int state, int x, int y) {
    if(button==GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
         if(glutGetModifiers()!=GLUT_ACTIVE_SHIFT) activateTool(button);
         else activateTool(GLUT_MIDDLE_BUTTON);
    }
    else if(button==GLUT_LEFT_BUTTON && state == GLUT_UP) deactivateTool();
    else if(button==GLUT_MIDDLE_BUTTON && state==GLUT_DOWN) { activateTool(button); }
    else if(button==GLUT_MIDDLE_BUTTON && state==GLUT_UP) deactivateTool();

    else if (button == 3) { // scroll up
        if (gameMode) {
            gameFOV -= 5;
            reshape(windowWidth, windowHeight);
        } else {
            viewDist = (viewDist < 0.0 ? viewDist : viewDist*0.8) - 0.05;
        }
    }
    else if(button == 4) { // scroll down
        if (gameMode) {
            gameFOV += 5;
            reshape(windowWidth, windowHeight);
        } else {
            viewDist = (viewDist < 0.0 ? viewDist : viewDist*1.25) + 0.05;
        }
    }
}

static void mousePassiveMotion(int x, int y) {
    if (gameMode) {
        if(mouseX < 50 || mouseX > windowWidth - 50 || mouseY < 50 || mouseY > windowHeight - 50) {
            glutWarpPointer(windowWidth/2, windowHeight/2);
        } else {
            yaw += (gameFOV/300.0f)*(x - mouseX) * mouseTurnScale;
            pitch += (gameFOV/300.0f)*(y - mouseY) * mouseTurnScale;
        }
    }
    mouseX=x;
    mouseY=y;
}


mat2 camRotZ() { return rotZ(-camRotSidewaysDeg) * mat2(10.0, 0, 0, -10.0); }


//---- callback functions for doRotate below and later
static void adjustCamrotsideViewdist(vec2 cv) {
    //cout << cv << endl;   // Debugging
    if (!gameMode)  {
        camRotSidewaysDeg+=cv[0]; 
        viewDist += cv[1]*10;
    }
}

static void adjustcamSideUp(vec2 su) { 
    if (!gameMode)  {
        camRotSidewaysDeg+=su[0];
        camRotUpAndOverDeg+=su[1];
    }
}
  
static void adjustLocXZ(vec2 xz) { 
    if (!gameMode)  {
        sceneObjs[toolObj].loc[0]+=xz[0];
        sceneObjs[toolObj].loc[2]+=xz[1];
    }
}

static void adjustScaleY(vec2 sy) 
{ 
    if (!gameMode)  {
        sceneObjs[toolObj].scale+=sy[0];
        sceneObjs[toolObj].loc[1]+=sy[1];
    }
}

//------Set the mouse buttons to rotate the camera around the centre of the scene. 
static void doRotate() {
    setToolCallbacks(adjustCamrotsideViewdist, mat2(400,0,0,-2),
                     adjustcamSideUp, mat2(400, 0, 0,-90) );
}
				   
//------Add an object to the scene

static void addObject(int id) {

  vec2 currPos = currMouseXYworld(camRotSidewaysDeg);
  sceneObjs[nObjects].loc[0] = currPos[0];
  sceneObjs[nObjects].loc[1] = 0.0;
  sceneObjs[nObjects].loc[2] = currPos[1];
  sceneObjs[nObjects].loc[3] = 1.0;

  if(id!=0 && id!=55)
      sceneObjs[nObjects].scale = 0.005;

  sceneObjs[nObjects].rgb[0] = 0.7; sceneObjs[nObjects].rgb[1] = 0.7;
  sceneObjs[nObjects].rgb[2] = 0.7; sceneObjs[nObjects].brightness = 1.0;
  sceneObjs[nObjects].alpha = 1.0;

  sceneObjs[nObjects].diffuse = 1.0; sceneObjs[nObjects].specular = 0.5;
  sceneObjs[nObjects].ambient = 0.7; sceneObjs[nObjects].shine = 10.0;

  sceneObjs[nObjects].angles[0] = 0.0; sceneObjs[nObjects].angles[1] = 180.0;
  sceneObjs[nObjects].angles[2] = 0.0;

  sceneObjs[nObjects].meshId = id;
  sceneObjs[nObjects].texId = rand() % numTextures;
  sceneObjs[nObjects].texScale = 2.0;

  toolObj = currObject = nObjects++;
  setToolCallbacks(adjustLocXZ, camRotZ(),
                   adjustScaleY, mat2(0.05, 0, 0, 10.0) );
  glutPostRedisplay();
}

// ------ The init function

void init( void )
{
    srand ( time(NULL) ); /* initialize random seed - so the starting scene varies */
    aiInit();

//    for(int i=0; i<numMeshes; i++)
//        meshes[i] = NULL;

    glGenVertexArrays(numMeshes, vaoIDs); CheckError(); // Allocate vertex array objects for meshes
    glGenTextures(numTextures, textureIDs); CheckError(); // Allocate texture objects

    // Load shaders and use the resulting shader program
    shaderProgram = InitShader( "src/vStart.glsl", "src/fStart.glsl" );

    glUseProgram( shaderProgram ); CheckError();

    // Initialize the vertex position attribute from the vertex shader    
    vPosition = glGetAttribLocation( shaderProgram, "vPosition" );
    vNormal = glGetAttribLocation( shaderProgram, "vNormal" ); CheckError();

    // Likewise, initialize the vertex texture coordinates attribute.  
    vTexCoord = glGetAttribLocation( shaderProgram, "vTexCoord" ); CheckError();

    // Likewise, initialize the vertex bone attributes.  
    vBoneIDs = glGetAttribLocation( shaderProgram, "vBoneIDs" );
    vBoneWeights = glGetAttribLocation( shaderProgram, "vBoneWeights" ); CheckError();

    projectionU = glGetUniformLocation(shaderProgram, "Projection");
    viewU = glGetUniformLocation(shaderProgram, "View");
    modelViewU = glGetUniformLocation(shaderProgram, "ModelView");
    boneTransformsU = glGetUniformLocation(shaderProgram, "BoneTransforms");

    // Objects 0, and 1 are the ground and the first light.
    addObject(0); // Square for the ground
    sceneObjs[0].loc = vec4(0.0, 0.0, 0.0, 1.0);
    sceneObjs[0].scale = 10.0;
    sceneObjs[0].angles[0] = 270.0; // Rotate it.
    sceneObjs[0].texScale = 5.0; // Repeat the texture.

    addObject(55); // Sphere for the first light
    sceneObjs[1].loc = vec4(2.0, 1.0, 1.0, 1.0);
    sceneObjs[1].scale = 0.1;
    sceneObjs[1].texId = 0; // Plain texture
    sceneObjs[1].brightness = 0.8; // The light's brightness is 5 times this (below).

    addObject(55); // Sphere for the first light
    sceneObjs[2].loc = vec4(1.0, 2.0, 1.0, 1.0);
    sceneObjs[2].scale = 0.3;
    sceneObjs[2].texId = 0; // Plain texture
    sceneObjs[2].brightness = 0.8; // The light's brightness is 5 times this (below).

    addObject(rand() % numMeshes); // A test mesh

    // We need to enable the depth test to discard fragments that
    // are behind previously drawn fragments for the same pixel.
    glEnable (GL_DEPTH_TEST);

    // Enable alpha blending.
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    doRotate(); // Start in camera rotate mode.
    glClearColor( 0.0, 0.0, 0.0, 1.0 ); /* black background */
}

//----------------------------------------------------------------------------

void drawMesh(SceneObject sceneObj, float pose_time) {

    // Activate a texture, loading if needed.
    loadTextureIfNotAlreadyLoaded(sceneObj.texId);
    glActiveTexture(GL_TEXTURE0 );
    glBindTexture(GL_TEXTURE_2D, textureIDs[sceneObj.texId]);

    // Texture 0 is the only texture type in this program, and is for the rgb colour of the
    // surface but there could be separate types for, e.g., specularity and normals. 
    glUniform1i( glGetUniformLocation(shaderProgram, "texture"), 0 );

    // Set the texture scale for the shaders
    glUniform1f( glGetUniformLocation( shaderProgram, "texScale"), sceneObj.texScale );

    // Set the projection matrix for the shaders
    glUniformMatrix4fv( projectionU, 1, GL_TRUE, projection );

    // Set the model matrix - this should combine translation, rotation and scaling based on what's
    // in the sceneObj structure (see near the top of the program).
    mat4 model = Translate(sceneObj.loc)*RotateZ(sceneObj.angles[2])*RotateY(sceneObj.angles[1])*RotateX(sceneObj.angles[0])*Scale(sceneObj.scale);

    // Set the model-view matrix for the shaders
    glUniformMatrix4fv( viewU, 1, GL_TRUE, view );
    glUniformMatrix4fv( modelViewU, 1, GL_TRUE, view * model );

    // Activate the VAO for a mesh, loading if needed.
    loadMeshIfNotAlreadyLoaded(sceneObj.meshId); CheckError();
    glBindVertexArray( vaoIDs[sceneObj.meshId] ); CheckError();

    int nBones = meshes[sceneObj.meshId]->mNumBones;
    if(nBones == 0)  nBones = 1;

    mat4 boneTransforms[64];
    calculateAnimPose(meshes[sceneObj.meshId], scenes[sceneObj.meshId], 0, pose_time, boneTransforms);
    glUniformMatrix4fv(boneTransformsU, nBones, GL_TRUE, (const GLfloat *)boneTransforms);

    glDrawElements(GL_TRIANGLES, meshes[sceneObj.meshId]->mNumFaces * 3, GL_UNSIGNED_INT, NULL); CheckError();
}


void display(void) {

    dt = glutGet(GLUT_ELAPSED_TIME) - t;

    // Prevent calculations where vsync timing hasn't occured
    // Note - this isn't true hardware vsync which communicates with the monitor,
    // (normally set in GPU config) though the improvement will remove a large amount of screen tearing.
    if(vsync && dt < 1000/Hz) return;
    numDisplayCalls++;
    t = glutGet(GLUT_ELAPSED_TIME);

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    CheckError(); // May report a harmless GL_INVALID_OPERATION with GLEW on the first frame

    // Set the view matrix.  To start with this just moves the camera backwards.  You'll need to
    // add appropriate rotations.

    if(gameMode) {

        // High school trig for position deltas (warning: pen and paper required).
        // Scaled by time delta and scaling factor afterwards.
        if (runForward) {
            dx -= sin(yaw*0.0174532925) * moveScale * dt;
            dz += cos(yaw*0.0174532925) * moveScale * dt;
        }
        if (runBack) {
            dx += sin(yaw*0.0174532925) * moveScale * dt;
            dz -= cos(yaw*0.0174532925) * moveScale * dt;
        }
        if (strafeRight) {
            dz -= sin(yaw*0.0174532925) * moveScale * dt;
            dx -= cos(yaw*0.0174532925) * moveScale * dt;
        }
        if (strafeLeft) {
            dz += sin(yaw*0.0174532925) * moveScale * dt;
            dx += cos(yaw*0.0174532925) * moveScale * dt;
        }

        // s = ut + (1/2)at^2
        // Luckily we can deal with s' through incremental +=/-= operators.
        // ds/dt = v = u + at
        if (jump) {
            dy += inertia * dt;
            inertia -= gravity * dt;
            if (dy < 1) {
                jump = false;
                dy = 1;
                inertia = impulse;
            }
        }

        // Keyboard tilting isn't forgotten for those not using the mouse.
        yaw -= yawLeft * turnScale * dt;
        yaw += yawRight * turnScale * dt;
        pitch -= pitchUp * turnScale * dt;
        pitch += pitchDown * turnScale * dt;

        // Limit the pitch to a single hemisphere to avoid nauseating effects of
        // gimble-lock outside this range (gimble-lock at 90 and -90 is desired though).
        if (pitch > 90) {
            pitch = 90;
        } else if (pitch < -90) {
            pitch = -90;
        }

        view = Translate(0.0, 0.0, 1) * RotateX(pitch) * RotateY(yaw) * Translate(dx, -dy, dz);

    } else {
        view = Translate(0.0, 0.0, 1-viewDist) * RotateX(camRotUpAndOverDeg) * RotateY(camRotSidewaysDeg);
    }

    SceneObject lightObj1 = sceneObjs[1]; 
    vec4 lightPosition1 = view * lightObj1.loc;

    SceneObject lightObj2 = sceneObjs[2]; 
    vec4 lightPosition2 = view * lightObj2.loc;

    glUniform4fv(glGetUniformLocation(shaderProgram, "LightPosition1"), 1, lightPosition1); CheckError();
    glUniform4fv(glGetUniformLocation(shaderProgram, "LightPosition2"), 1, lightPosition2); CheckError();

    for(int i=0; i<nObjects; i++) {
        SceneObject so = sceneObjs[i];

        if (hidden[i]) {
          continue;
        }

        glUniform1f(glGetUniformLocation(shaderProgram, "Alpha"), so.alpha );

        vec3 rgb1 = so.rgb * lightObj1.rgb * so.brightness * lightObj1.brightness;
        glUniform3fv(glGetUniformLocation(shaderProgram, "AmbientProduct1"), 1, so.ambient * rgb1 ); CheckError();
        glUniform3fv(glGetUniformLocation(shaderProgram, "DiffuseProduct1"), 1, so.diffuse * rgb1 );
        glUniform3fv(glGetUniformLocation(shaderProgram, "SpecularProduct1"), 1, so.specular * rgb1 );

        vec3 rgb2 = so.rgb * lightObj2.rgb * so.brightness * lightObj2.brightness;
        glUniform3fv(glGetUniformLocation(shaderProgram, "AmbientProduct2"), 1, so.ambient * rgb2 ); CheckError();
        glUniform3fv(glGetUniformLocation(shaderProgram, "DiffuseProduct2"), 1, so.diffuse * rgb2 );
        glUniform3fv(glGetUniformLocation(shaderProgram, "SpecularProduct2"), 1, so.specular * rgb2 );

        glUniform1f(glGetUniformLocation(shaderProgram, "Shininess"), so.shine ); CheckError();

        drawMesh(sceneObjs[i], (float) numDisplayCalls);
    }

    glutSwapBuffers();

}

//--------------Menus

static void objectMenu(int id) {
  deactivateTool();
  addObject(id);
}

static void texMenu(int id) {
    deactivateTool();
    if(currObject>=0) {
        sceneObjs[currObject].texId = id;
        glutPostRedisplay();
    }
}

static void groundMenu(int id) {
        deactivateTool();
        sceneObjs[0].texId = id;
        glutPostRedisplay();
}

static void adjustBrightnessY(vec2 by) 
  { sceneObjs[toolObj].brightness+=by[0]; sceneObjs[toolObj].loc[1]+=by[1]; }

static void adjustRedGreen(vec2 rg) 
  { sceneObjs[toolObj].rgb[0]+=rg[0]; sceneObjs[toolObj].rgb[1]+=rg[1]; }

static void adjustBlueBrightness(vec2 bl_br) 
  { sceneObjs[toolObj].rgb[2]+=bl_br[0]; sceneObjs[toolObj].brightness+=bl_br[1]; }

  static void lightMenu(int id) {
    deactivateTool();
    if(id == 70) {
	    toolObj = 1;
        setToolCallbacks(adjustLocXZ, camRotZ(),
                         adjustBrightnessY, mat2( 1.0, 0.0, 0.0, 10.0) );

    } else if(id>=71 && id<=74) {
	    toolObj = 1;
        setToolCallbacks(adjustRedGreen, mat2(1.0, 0, 0, 1.0),
                         adjustBlueBrightness, mat2(1.0, 0, 0, 1.0) );
    } else if(id == 80) {
        toolObj = 2;
        setToolCallbacks(adjustLocXZ, camRotZ(),
                         adjustBrightnessY, mat2( 1.0, 0.0, 0.0, 10.0) );

    } else if(id>=81 && id<=84) {
        toolObj = 2;
        setToolCallbacks(adjustRedGreen, mat2(1.0, 0, 0, 1.0),
                         adjustBlueBrightness, mat2(1.0, 0, 0, 1.0) );
    }

    else { printf("Error in lightMenu\n"); exit(1); }
}

static int createArrayMenu(int size, const char menuEntries[][128], void(*menuFn)(int)) {
    int nSubMenus = (size-1)/10 + 1;
    int subMenus[nSubMenus];

    for(int i=0; i<nSubMenus; i++) {
        subMenus[i] = glutCreateMenu(menuFn);
        for(int j = i*10+1; j<=min(i*10+10, size); j++)
     glutAddMenuEntry( menuEntries[j-1] , j); CheckError();
    }
    int menuId = glutCreateMenu(menuFn);

    for(int i=0; i<nSubMenus; i++) {
        char num[6];
        sprintf(num, "%d-%d", i*10+1, min(i*10+10, size));
        glutAddSubMenu(num,subMenus[i]); CheckError();
    }
    return menuId;
}

static void adjustAmbientDiffuse(vec2 ad) {
  sceneObjs[toolObj].ambient += ad[0];
  sceneObjs[toolObj].diffuse += ad[1];
}

static void adjustSpecularShine(vec2 ss) {
  sceneObjs[toolObj].specular += ss[0];
  sceneObjs[toolObj].shine += ss[1] * 10;
}

static void adjustAlpha(vec2 by) {
  sceneObjs[toolObj].alpha += by[0];
}

static void materialMenu(int id) {
  deactivateTool();
  if(currObject<0) return;

  if(id==10) {
    toolObj = currObject;
    setToolCallbacks(adjustRedGreen, mat2(1, 0, 0, 1),
                     adjustBlueBrightness, mat2(1, 0, 0, 1) );
  }

  if(id==20) {
    toolObj = currObject;
    setToolCallbacks(adjustAmbientDiffuse, mat2(1, 0, 0, 1),
                     adjustSpecularShine, mat2(1, 0, 0, 1) );
  }

  if(id==30) {
    toolObj = currObject;
    setToolCallbacks(adjustAlpha, mat2(1, 0, 0, 1),
                     adjustAlpha, mat2(1, 0, 0, 1) );
  }
                      
  else { printf("Error in materialMenu\n"); }
}

static void selectMenu(int id) {
  if (id == 2) {
    toolObj++;

    if (toolObj == nObjects) {
      toolObj = 0;
    }
  } else {
    if (toolObj == 0) {
      toolObj = nObjects - 1;
    } else {
      toolObj--;
    }
  }

  currObject = toolObj;
}

static void adjustAngleYX(vec2 angle_yx) 
  {  sceneObjs[currObject].angles[1]+=angle_yx[0]; sceneObjs[currObject].angles[0]+=angle_yx[1]; }

static void adjustAngleZTexscale(vec2 az_ts) 
  {  sceneObjs[currObject].angles[2]+=az_ts[0]; sceneObjs[currObject].texScale+=az_ts[1]; }


static void mainmenu(int id) {
    deactivateTool();
    if(id == 41 && currObject>=0) {
	    toolObj=currObject;
        setToolCallbacks(adjustLocXZ, camRotZ(),
                         adjustScaleY, mat2(0.05, 0, 0, 10) );
    }
    if(id == 45) {
        switchMode();
    }
    if(id == 50)
        doRotate();
    if(id == 55 && currObject>=0) {
        setToolCallbacks(adjustAngleYX, mat2(400, 0, 0, -400),
                         adjustAngleZTexscale, mat2(400, 0, 0, 15) );
    }
    if (id == 56) {
      hidden[toolObj] = 1;
    }
    if (id == 57) {
      hidden[toolObj] = 0;
    }
    if (id == 58) {
        sceneObjs[nObjects] = sceneObjs[currObject];

        toolObj = currObject = nObjects++;
    }
    if(id == 99) exit(0);
}

static void makeMenu() {
  int objectId = createArrayMenu(numMeshes, objectMenuEntries, objectMenu);

  int materialMenuId = glutCreateMenu(materialMenu);
  glutAddMenuEntry("R/G/B/All",10);
  glutAddMenuEntry("Ambient/Diffuse/Specular/Shine",20);
  glutAddMenuEntry("Alpha",30);

  int texMenuId = createArrayMenu(numTextures, textureMenuEntries, texMenu);
  int groundMenuId = createArrayMenu(numTextures, textureMenuEntries, groundMenu);

  int lightMenuId = glutCreateMenu(lightMenu);
  glutAddMenuEntry("Move Light 1",70);
  glutAddMenuEntry("R/G/B/All Light 1",71);
  glutAddMenuEntry("Move Light 2",80);
  glutAddMenuEntry("R/G/B/All Light 2",81);

  selectMenuId = glutCreateMenu(selectMenu);
  glutAddMenuEntry("Previous Object", 1);
  glutAddMenuEntry("Next Object", 2);

  glutCreateMenu(mainmenu);
  glutAddMenuEntry("Toggle Game Mode",45);
  glutAddMenuEntry("Rotate/Move Camera",50);
  glutAddSubMenu("Add object", objectId);
  glutAddSubMenu("Select object", selectMenuId);
  glutAddMenuEntry("Duplicate object", 58);
  glutAddMenuEntry("Hide object", 56);
  glutAddMenuEntry("Unide object", 57);
  glutAddMenuEntry("Position/Scale", 41);
  glutAddMenuEntry("Rotation/Texture Scale", 55);
  glutAddSubMenu("Material", materialMenuId);
  glutAddSubMenu("Texture",texMenuId);
  glutAddSubMenu("Ground Texture",groundMenuId);
  glutAddSubMenu("Lights",lightMenuId);
  glutAddMenuEntry("EXIT", 99);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
}


//----------------------------------------------------------------------------

void
normalKeyboardDown( unsigned char key, int x, int y )
{
    // Looked up all of these on an ASCII chart initially.
    // That would have looked silly.
    switch ( key ) {
    case 'w':
        runForward = true;
        break;
    case 'a':
        strafeLeft = true;
        break;
    case 's':
        runBack = true;
        break;
    case 'd':
        strafeRight = true;
        break;
    }
}


//----------------------------------------------------------------------------

void
specialKeyboardDown( int key, int x, int y )
{
    switch ( key ) {
    case GLUT_KEY_UP:
        pitchUp = true;
        break;
    case GLUT_KEY_DOWN:
        pitchDown = true;
        break;
    case GLUT_KEY_LEFT:
        yawLeft = true;
        break;
    case GLUT_KEY_RIGHT:
        yawRight = true;
        break;
    case GLUT_KEY_PAGE_UP:
        if (gameMode) {
            gameFOV -= 20;
            reshape(windowWidth, windowHeight);
        } else {
            viewDist = (viewDist < 0.0 ? viewDist : viewDist*0.8) - 0.05;
        }
        break;
    case GLUT_KEY_PAGE_DOWN:
        if (gameMode) {
            gameFOV += 20;
            reshape(windowWidth, windowHeight);
        } else {
            viewDist = (viewDist < 0.0 ? viewDist : viewDist*1.25) + 0.05;
        }
        break;
    }
}

//----------------------------------------------------------------------------

void
normalKeyboardUp( unsigned char key, int x, int y )
{
    switch ( key ) {
    case 'w':
        runForward = false;
        break;
    case 'a':
        strafeLeft = false;
        break;
    case 's':
        runBack = false;
        break;
    case 'd':
        strafeRight = false;
        break;
    case 'v':
        vsync = !vsync;
        break;
    case 'g':
        switchMode();
        break;
    case 'f':
        toggleFullScreen();
        break;
    case ' ':
        jump = true;
        break;
    case 27:
        exit( EXIT_SUCCESS );
        break;
    }
}


//----------------------------------------------------------------------------

void
specialKeyboardUp( int key, int x, int y )
{
    switch ( key ) {
    case GLUT_KEY_LEFT:
        yawLeft = false;
        break;
    case GLUT_KEY_RIGHT:
        yawRight = false;
        break;
    case GLUT_KEY_UP:
        pitchUp = false;
        break;
    case GLUT_KEY_DOWN:
        pitchDown = false;
        break;
    }
}

//----------------------------------------------------------------------------


void idle( void ) {
  glutPostRedisplay();
}

void timer(int unused)
{
    char title[256];
    char prefix[13];
    if (vsync) {
        sprintf(prefix, "VSYNC ON -- ");
    } else {
        sprintf(prefix, "VSYNC OFF -- ");
    }
    sprintf(title, "%s %s %s: %d Frames Per Second @ %d x %d", prefix,
            lab, programName, numDisplayCalls, windowWidth, windowHeight );

    glutSetWindowTitle(title);

    numDisplayCalls = 0;
    glutTimerFunc(1000, timer, 1);
}


void fileErr(char* fileName) {
    printf("Error reading file: %s\n\n", fileName);

    printf("Download and unzip the models-textures folder - either:\n");
    printf("a) as a subfolder here (on your machine)\n");
    printf("b) at c:\\temp\\models-textures (labs Windows)\n");
    printf("c) or /tmp/models-textures (labs Linux).\n\n");
    printf("Alternatively put to the path to the models-textures folder on the command line.\n");

    exit(1);
}

int main( int argc, char* argv[] )
{
    // Get the program name, excluding the directory, for the window title
    programName = argv[0];
    for(char *cpointer = argv[0]; *cpointer != 0; cpointer++)
        if(*cpointer == '/' || *cpointer == '\\') programName = cpointer+1;

    // Set the models-textures directory, via the first argument or some handy defaults.
    if(!opendir(dataDir)) fileErr(dataDir);

    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );
    glutInitWindowSize( windowWidth, windowHeight );

    glutInitContextVersion(3, 2);
    //glutInitContextProfile( GLUT_CORE_PROFILE );        // May cause issues, sigh, but you
    glutInitContextProfile( GLUT_COMPATIBILITY_PROFILE ); // should still use only OpenGL 3.2 Core
                                                          // features.
    glutCreateWindow( "Initialising..." );

    glewInit(); // With some old hardware yields GL_INVALID_ENUM, if so use glewExperimental.
    CheckError(); // This bug is explained at: http://www.opengl.org/wiki/OpenGL_Loading_Library

    makeMenu(); CheckError();

    init(); CheckError(); // Use CheckError after an OpenGL command to print any errors.

    glutDisplayFunc(display);
    glutKeyboardFunc(normalKeyboardDown);
    glutSpecialFunc(specialKeyboardDown);
    glutKeyboardUpFunc(normalKeyboardUp);
    glutSpecialUpFunc(specialKeyboardUp); 
    glutIdleFunc(idle);

    glutMouseFunc( mouseClickOrScroll );
    glutPassiveMotionFunc(mousePassiveMotion);
    glutMotionFunc( doToolUpdateXY );

    glutSetKeyRepeat( GLUT_KEY_REPEAT_OFF ); // Cannot jump by holding, scope zooms in 
                                             // levels as opposed to continuous (arbitrary).
 
    glutReshapeFunc( reshape );
    glutTimerFunc(1000, timer, 1);   CheckError();

    glutMainLoop();
    return 0;
}
