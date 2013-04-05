/* 
 * File:   main.cpp
 * Author: tomma_000
 *
 * Created on February 2, 2013, 2:23 PM
 */

#include <cstdlib>
#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>
#include <GL/glew.h>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include "shaders.h"
#include <math.h>
#include <map>
#include <queue>
#include <algorithm>
#include <stack>
using namespace std;

int maxVert = 0;
int maxFace = 0;
int maxEdge = 0;
vector< vector<float> > allVerts;
vector< vector<int> > allFaces;
vector< vector<int> > allEdges;

vector< vector<float> > trueAllEdges;
vector< vector<float> > trueAllFaces;

stack < vector< vector<int> > > reverseEdges;
stack < vector< vector<int> > > reverseFaces;
stack < vector< vector<float> > > reverseVerts2;

map < vector<int>, bool > edgeCheck;

map < int, glm::mat4 > errorMatrix;
map < int, float > errors;
vector < vector<float> > pairOrder;

GLuint istex ;
GLuint islight ;
GLuint light0dirn ; 
GLuint light0color ; 
GLuint light1posn ; 
GLuint light1color ; 
GLuint ambient ; 
GLuint diffuse ; 
GLuint specular ; 
GLuint shininess ; 
GLuint vertexshader, fragmentshader, shaderprogram ;

float nx, ny, nz;
int mouseoldx, mouseoldy;
float anglex, angley;
float scale;
bool isRotate = false;
bool isScale = false;
float oldscale;
int progressive = 0;
string inputFile;

int showAndTell = 0;

void parseOFFFile(string input);

void keyboard (unsigned char key, int x, int y) {
  switch (key) {
      case 'r': // reset eye and up vectors, scale and translate. 
          progressive = 0;
	  break ;   
      case 27:  // Escape to quit
	  exit(0) ;
	  break ;
      case 61:
          progressive++;
          break;
      case 45:
          progressive--;
          break;
      case 's':
          showAndTell = 1;
          break;
      case 'w':
          showAndTell = 0;
          break;
      default:
      	break ;
  }
  glutPostRedisplay();
}
void specialKey(int key, int x, int y) {
	switch(key) {
            case 100: //left
                nx -= 0.1f;
                break;
            case 103: //down
                ny -= 0.1f;
                break;
	    case 102: //right
                nx += 0.1f;
                break;
	    case 101: //up
                ny += 0.1f;
                break;
            default:
                nx = 0;
                ny = 0;
                break;
	}
	glutPostRedisplay();
}
void mouse(int button, int state, int x, int y) {
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_UP) {
        isRotate = false;
    }
    else if (state == GLUT_DOWN) {
        mouseoldx = x; mouseoldy = y;
        isRotate = true;
    }
  }
  if (button == GLUT_RIGHT_BUTTON) {
    if (state == GLUT_UP) {
        isScale = false;
        oldscale = scale;
    }
    else if (state == GLUT_DOWN) {
        isScale = true;
        mouseoldx = x; mouseoldy = y;
    }
  }
  glutPostRedisplay();
}
void mousedrag(int x, int y) {
    
    int newX = x - mouseoldx;
    int newY = y - mouseoldy;
    
    if (isRotate) {
        anglex = -(float) (newX * 0.5f);
        angley = -(float) (newY*0.5f);
    }
    
    if (isScale) {
        float sNewY = (float) y - (float) mouseoldy;
        
        scale = oldscale + sNewY * 0.01;
        
        if (scale < 0.0) {
            scale = 0.0;
        }
    }
    
    glutPostRedisplay();
}
void transformvec (const GLfloat input[4], GLfloat output[4]) {
  GLfloat modelview[16] ; // in column major order
  glGetFloatv(GL_MODELVIEW_MATRIX, modelview) ; 
  
  for (int i = 0 ; i < 4 ; i++) {
    output[i] = 0 ; 
    for (int j = 0 ; j < 4 ; j++) 
      output[i] += modelview[4*j+i] * input[j] ; 
  }
}

void assignError() {
    for (int i=0; i<allVerts.size(); i++) {
        glm::vec4 v(allVerts[i][0],allVerts[i][1],allVerts[i][2],1);
        errors[i] = glm::dot(v,errorMatrix[i]*v);
    }
}
bool heapOrder(vector<float> v1, vector<float> v2) {
    return v2[2] < v1[2];
}
void assignOrder() {
    for (int i=0; i<allEdges.size(); i++) {
        vector<float> r;
        float v1 = (float) allEdges[i][0];
        float v2 = (float) allEdges[i][1];
        r.push_back(v1);
        r.push_back(v2);
        r.push_back(errors[allEdges[i][0]]+errors[allEdges[i][1]]);
        pairOrder.push_back(r);
    }
    make_heap(pairOrder.begin(), pairOrder.end(), heapOrder);
}
void updateError(int v, glm::mat4 k) {
    glm::mat4 newvalue;
    if (errorMatrix.find(v) != errorMatrix.end()) {
        newvalue = errorMatrix[v]+k;
    } else {
        newvalue = k;
    }
    errorMatrix[v] = newvalue;
}
void makeQuadric(vector<int> face) {
    glm::vec3 a(allVerts[face[1]][0],allVerts[face[1]][1],allVerts[face[1]][2]);
    glm::vec3 b(allVerts[face[2]][0],allVerts[face[2]][1],allVerts[face[2]][2]);
    glm::vec3 c(allVerts[face[3]][0],allVerts[face[3]][1],allVerts[face[3]][2]);
    glm::vec3 ab = b-a;
    glm::vec3 bc = c-b;
    for (int i=1; i<4; i++) {
        glm::vec3 v(allVerts[face[i]][0],allVerts[face[i]][1],allVerts[face[i]][2]);
        float d = -glm::dot(a, v);
        glm::vec4 n(glm::normalize(glm::cross(ab, bc)), d);
        float na = n[0];
        float nb = n[1];
        float nc = n[2];
        float nd = n[3];
        glm::mat4 k(na*na,na*nb,na*nc,na*nd,nb*na,nb*nb,nb*nc,nb*nd,nc*na,nc*nb,nc*nc,nc*nd,nd*na,nd*nb,nd*nc,nd*nd);
        updateError(face[i],k);
    }
}

void parseOFFLineVert(string input) {
    istringstream pInput(input);
    string token;
    vector<float> vertex;
    while (getline(pInput, token, ' ')) {
        vertex.push_back(atof(token.c_str()));
    }
    allVerts.push_back(vertex);
}
bool contains(vector<int> item) {
    bool result = false;
    vector<int> reverseItem;
    for (int i = item.size()-1; i >= 0; i--) {
        reverseItem.push_back(item[i]);
    }
    
    if ((edgeCheck[item] == true) || (edgeCheck[reverseItem] == true)) {
        return true;
    } else {
        edgeCheck[item] = true;
        return false;
    }
}
void parseOFFLineFace(string input) {
    istringstream pInput(input);
    string token;
    vector<int> face;
    getline(pInput, token, ' ');
    int numVerts = atoi(token.c_str());
    face.push_back(numVerts);
    
    for (int i = 0; i < numVerts; i++) {
        getline(pInput, token, ' ');
        int vert = atoi(token.c_str());
        face.push_back(vert);
    }
    
    for (int i = 1; i <= numVerts; i++) {
        vector<int> edge;
        if (i==numVerts) {
            edge.push_back(face[numVerts]);
            edge.push_back(face[1]);
        } else {
            edge.push_back(face[i]);
            edge.push_back(face[i+1]);
        }
        if (!contains(edge)) {
            allEdges.push_back(edge);
        }
    }
    
    makeQuadric(face);
    
    allFaces.push_back(face);
}
bool isOFF(string input) {
    if (strcmp(input.c_str(), "OFF") == 0) {
        return true;
    }
    return false;
}
void parseVertFace(string input) {
    istringstream pInput(input);
    string token;
    getline(pInput, token, ' ');
    maxVert = atoi(token.c_str());
    getline(pInput, token, ' ');
    maxFace = atoi(token.c_str());
    getline(pInput, token, ' ');
    maxEdge = atoi(token.c_str());
}
void parseOFFFile(string input) {
    ifstream myfile;
    string line;
    bool firstLine = true;
    myfile.open(input.c_str());
    int vcounter = 0;
    int fcounter = 0;
    
    while (!myfile.eof()) {
        
        if (firstLine) {
            getline (myfile, line);
            if (! isOFF(line)) {
                cout << "Not an OFF file" << endl;
                break;
            }
            getline (myfile, line);
            parseVertFace(line);
            firstLine = false;
        }
        if (vcounter < maxVert) {
            getline (myfile, line);
            parseOFFLineVert(line);
            vcounter ++;
        } else if (fcounter < maxFace) {
            getline (myfile, line);
            parseOFFLineFace(line);
            fcounter ++;
        } else {
            break;
            // DON"T DO ANYTHING FOR EDGE
        }
    }
    myfile.close();
    
}

void mapVertEdge() {
    
}
void mapVertFace() {
    
}
void reOrgEdges() {
    for (int i = 0; i < allEdges.size(); i++) {
        vector<float> temp;
        for (int j = 0; j < allEdges[i].size(); j++) {
            //temp.push_back(allVerts[allEdges[i][j]]);
        }
        trueAllEdges.push_back(temp);
    }
}
void reOrgItems() {
    reOrgEdges();
    cout << trueAllEdges.size() << endl;
    cout << allEdges.size() << endl;
}

void init() {
   
   glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

   glutInitWindowSize (500, 500); 
   glutInitWindowPosition (100, 100);
   glutCreateWindow ("CS283 Progresive Meshes");
   
   /* INITIALIZE TRANSLATION/ROTATION VARIABLES*/
   nx = 0;
   ny = 0;
   nz = 0;
   mouseoldx = 0;
   mouseoldy = 0;
   anglex = 0;
   angley = 0;
   scale = 1.0f;
   oldscale = 1.0f;

   GLenum err = glewInit() ; 
   if (GLEW_OK != err) { 
	std::cerr << "Error: " << glewGetString(err) << std::endl; 
   }
   
   glClearColor (0.0, 0.0, 0.0, 0.0);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   gluLookAt(0, 0, 0, 0, 0, 5.0f, 0, 1.0f, 0) ;
   
   vertexshader = initshaders(GL_VERTEX_SHADER, "shaders/light.vert.glsl") ;
   fragmentshader = initshaders(GL_FRAGMENT_SHADER, "shaders/light.frag.glsl") ;
   shaderprogram = initprogram(vertexshader, fragmentshader) ; 
   islight = glGetUniformLocation(shaderprogram,"islight") ;        
   light0dirn = glGetUniformLocation(shaderprogram,"light0dirn") ;       
   light0color = glGetUniformLocation(shaderprogram,"light0color") ;       
   light1posn = glGetUniformLocation(shaderprogram,"light1posn") ;       
   light1color = glGetUniformLocation(shaderprogram,"light1color") ;       
   ambient = glGetUniformLocation(shaderprogram,"ambient") ;       
   diffuse = glGetUniformLocation(shaderprogram,"diffuse") ;       
   specular = glGetUniformLocation(shaderprogram,"specular") ;       
   shininess = glGetUniformLocation(shaderprogram,"shininess") ;    
   glEnable(GL_DEPTH_TEST) ;
   glDepthFunc (GL_LESS) ;
   
   
   
}
void shade() {
     const GLfloat one[] = {1, 1, 1, 1};
     const GLfloat medium[] = {0.5, 0.5, 0.5, 1};
     const GLfloat small[] = {0.2, 0.2, 0.2, 1};
     const GLfloat high[] = {100} ;
     const GLfloat zero[] = {0.0, 0.0, 0.0, 1.0} ; 
     const GLfloat light_specular[] = {1, 0.5, 0, 1};
     const GLfloat light_specular1[] = {0, 0.5, 1, 1};
     const GLfloat light_direction[] = {0.5, 0, 0, 0}; // Dir light 0 in w 
     const GLfloat light_position1[] = {0, -0.5, 0, 1};

     GLfloat light0[4], light1[4] ; 

     transformvec(light_direction, light0) ; 
     transformvec(light_position1, light1) ; 
 
     glUniform3fv(light0dirn, 1, light0) ; 
     glUniform4fv(light0color, 1, light_specular) ; 
     glUniform4fv(light1posn, 1, light1) ; 
     glUniform4fv(light1color, 1, light_specular1) ; 

     glUniform4fv(ambient,1,small) ; 
     glUniform4fv(diffuse,1,medium) ; 
     glUniform4fv(specular,1,one) ; 
     glUniform1fv(shininess,1,high) ; 
     GLint lighting = 1;
     glUniform1i(islight,lighting) ;
}

void drawVertex(int index) {
    glVertex3f(allVerts[index][0], allVerts[index][1], allVerts[index][2]);
}
void drawTriangle(int index) {
    glBegin(GL_TRIANGLES);
    glColor3f(1.0, 1.0, 1.0);
    for (int i = 1; i <= 3; i++) {
       drawVertex(allFaces[index][i]);
    }
    glEnd();
}
void drawFaces() {
    for (int i = 0; i < allEdges.size(); i++) {
        glLineWidth(2.0);
        glBegin(GL_LINES);
        glColor3f(0.0, 1.0, 0.0);
        drawVertex(allEdges[i][0]);
        drawVertex(allEdges[i][1]);
        glEnd();
    }
    for (int i = 0; i < allFaces.size(); i++) {
        if (allFaces[i][0] == 3) {
            drawTriangle(i);
            //drawLines(i);
        }
    }

}

/* This is a debugging function to see which edge we are collapsing*/
void showEdge(int index) {
    vector<int> edge = allEdges[index];
    glLineWidth(5.0);
    glBegin(GL_LINES);
    glColor3f(1.0, 0.0, 0.0);
    drawVertex(edge[0]);
    drawVertex(edge[1]);
    glEnd();
}

void changeEdge(int stay, int out) {
    /* Go through allEdges and change out to stay*/
    bool isStay = false;
    bool isOut = false;
    
    for (int i = 0; i < allEdges.size(); i++) {
        for (int j = 0; j < allEdges[i].size(); j++) {
            if (allEdges[i][j] == out) {
                allEdges[i][j] = stay;
                isOut = true;
            }
            else if (allEdges[i][j] == stay) {
                isStay = true;
            }
        }
        
        if (isStay && isOut) {
            allEdges.erase(allEdges.begin() + i);
        }
        
        isStay = false;
        isOut = false;
    }
}
bool newContains(vector< vector<int> > checked, vector<int> item) {
    bool result = false;
    vector<int> reverseItem;
    for (int i = item.size()-1; i >= 0; i--) {
        reverseItem.push_back(item[i]);
    }
    
    for (int j = 0; j < checked.size(); j++) {
        if ((checked[j] == item) || (checked[j] == reverseItem)) {
            return true;
        }
    }
    
    return false;
    
}
void changeEdge2() {
    vector< vector<int> > newAllEdges;
    for (int i = 0; i < allFaces.size(); i++) {
        for (int j = 1; j < allFaces[i].size(); j++) {
            vector<int> newEdge;
            if (j == (allFaces[i].size() - 1)) {
                newEdge.push_back(allFaces[i][j]);
                newEdge.push_back(allFaces[i][1]);
            } else {
                newEdge.push_back(allFaces[i][j]);
                newEdge.push_back(allFaces[i][j+1]);
            }
            
            if (!newContains(newAllEdges, newEdge)) {
                newAllEdges.push_back(newEdge);
            }
        }
    }
    allEdges = newAllEdges;
}
void changeFace(int stay, int out) {
    /* Go thoguh allFaces and change out to stay*/
    bool isStay = false;
    bool isOut = false;
    
    for (int i = 0; i < allFaces.size(); i++) {
         for (int j = 1; j < allFaces[i].size(); j++) {
             if (allFaces[i][j] == out) {
                 allFaces[i][j] = stay;
                 isOut = true;
             }
             else if (allFaces[i][j] == stay) {
                 isStay = true;
             }
         }
         
         if (isStay && isOut) {
             allFaces.erase(allFaces.begin()+i);
         }
         isStay = false;
         isOut = false;
     }
    changeEdge2();
}
void edgeCollapsev2(vector<int> edgez2) {
    // THIS IS A LINEAR TIME VERSION
    /*SO GENERAL IDEA, get a random edge, and go though and change things then clean up edges and faces*/
    
    vector<int> edgeCollapsing = edgez2;
    int stayInt = edgez2[0];
    int outInt = edgez2[1];
    vector<float> v1 = allVerts[stayInt];
    vector<float> v2 = allVerts[outInt];
    reverseVerts2.push(allVerts);
    reverseFaces.push(allFaces);
    reverseEdges.push(allEdges);
    
    vector<float> avgVert;
    
    for (int i = 0; i < v1.size(); i++) {
        float temp = (v1[i] + v2[i])/2;
        avgVert.push_back(temp);
    }
    
    for (int j = 0; j < v1.size(); j++) {
        allVerts[stayInt][j] = avgVert[j];
        //allVerts[outInt][j] = avgVert[j];
    }
    
    //changeEdge(stayInt, outInt);
    changeFace(stayInt, outInt);
    // I don't think we can remove the vertex or else it ends up screwing up with our order
    //changeVertex(stayInt, outInt);
    glutPostRedisplay();
}
void reverseEdgeColl() {
    //THIS IS TO REVERT THE VERTS
    if (!(reverseVerts2.empty())){
        allVerts = reverseVerts2.top();
        reverseVerts2.pop();
    }
    
    //THIS IS TO PUT BAD THE EDGES
    if (!(reverseEdges.empty())) {
        allEdges = reverseEdges.top();
        reverseEdges.pop();
    }
    //THIS IS TO PUT BACK THE FACES
    if (!(reverseFaces.empty())) {
        allFaces = reverseFaces.top();
        reverseFaces.pop();
    }
}

void display() {
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT) ; 
    glMatrixMode(GL_MODELVIEW) ; 
    
    /* DRAW THE OBJECT HERE*/

    //shade();
    
    glPushMatrix();
    glTranslatef(nx, ny, nz);
    glScalef(scale, scale, scale);
    glRotatef(anglex, 0.0, 1.0, 0.0);
    glRotatef(angley, 1.0, 0.0, 0.0);
    
    drawFaces();
    
    if (progressive > 0 && (allEdges.size() > 0)) {
        
        vector<float> v;
        vector<int> edge;
        if (pairOrder.size() > 0) {
            vector<int> vint;
            v = pairOrder.front();
            pop_heap(pairOrder.begin(), pairOrder.end(), heapOrder);
            pairOrder.pop_back();
            int vint1 = (int) v[0];
            int vint2 = (int) v[1];
            vint.push_back(vint1);
            vint.push_back(vint2);
            while (pairOrder.size() > 0 && !contains(vint)) {
                v = pairOrder.front();
                pop_heap(pairOrder.begin(), pairOrder.end(), heapOrder);
                pairOrder.pop_back();
                int vint3 = (int) v[0];
                int vint4 = (int) v[1];
                vint.pop_back();
                vint.pop_back();
                vint.push_back(vint3);
                vint.push_back(vint4);
            }
            if (contains(vint)) {
                int v1 = (int) v[0];
                int v2 = (int) v[1];
            
                edge.push_back(v1);
                edge.push_back(v2);
            } else {
                int randm = rand() % allEdges.size();
                edge = allEdges[randm];
            }
        } else {
            int randm = rand() % allEdges.size();
            edge = allEdges[randm];
            //edgeCollapsev2(allEdges[randm]);
        }
        edgeCollapsev2(edge);
        progressive = 0;
    }
    if (progressive < 0) {
        reverseEdgeColl();
        progressive = 0;
        glutPostRedisplay();
    }
    //cout << allEdges.size() << endl;
    //cout << allFaces.size() << endl;
    //cout << allVerts.size() << endl;
    glPopMatrix();
    glutSwapBuffers();
    glFlush();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    //cin >> inputFile;
    init();
    parseOFFFile("plane.off");
    assignError();
    assignOrder();
    reOrgItems();
    
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKey);
    glutMouseFunc(mouse) ;
    glutMotionFunc(mousedrag) ;
    glutMainLoop();
    
    return 0;
}

