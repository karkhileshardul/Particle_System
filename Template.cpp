#include<windows.h>
#include<stdio.h>
#include<gl\glew.h>
#include<gl/GL.h>
#include"vmath.h"

#define PI 3.146
#define WIN_WIDTH	800
#define WIN_HEIGHT	600

#define SSK_VERTEX_ARRAY		1
#define SSK_COLOR_ARRAY			2
#define SSK_VELOCITY_ARRAY		3
#define SSK_START_TIME_ARRAY	4



#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"opengl32.lib")

enum{
	SSK_ATTRIBUTE_VERTEX=0,
	SSK_ATTRIBUTE_COLOR,
	SSK_ATTRIBUTE_NORMAL,
	SSK_ATTRIBUTE_TEXTURE0,
};

FILE *gpFile=NULL;
HWND ghwnd;
bool gbFullscreen=false;
bool gbActiveWindow=false;
bool gbEscapeKeyIsPressed=false;
DWORD dwStyle;
WINDOWPLACEMENT wpPrev={sizeof(WINDOWPLACEMENT)};
vmath::mat4 gPerspectiveProjectionMatrix;
HDC ghdc;
HGLRC ghrc;
GLuint gVertexShaderObject;
GLuint gFragmentShaderObject;
GLuint gShaderProgramObject;

GLuint gVao;
GLuint gVbo;
GLuint gVbo2;
GLuint gVbo3;
GLuint gVbo4;
GLuint gMVPUniform;
GLuint location;
GLuint Color1;
GLuint Color2;
GLuint Color3;
GLfloat ParticleTime=0.1f;
GLfloat angle=0.1f;

static GLint arrayWidth, arrayHeight;
static GLfloat *verts = NULL;
static GLfloat *colors = NULL;
static GLfloat *velocities = NULL;
static GLfloat *startTimes = NULL;

LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpszCmdLine,int iCmdShow){
	WNDCLASSEX wndclassex;
	MSG msg;
	TCHAR szClassName[]=TEXT("09-Particle System Window");
	HWND hwnd;
	bool bDone=false;
	void initialize(void);
	void uninitialize(void);
	void display(void);

	if(fopen_s(&gpFile,"SSK_Log File.txt","w")!=0){
		MessageBox(NULL,TEXT("Log file error"),TEXT("Log File cannot be created\n...Exiting Now"),MB_OK)	;
		exit(EXIT_FAILURE);
	}else{
		fprintf(gpFile,"Log File is Successfully Opened\n");
	}

	wndclassex.cbSize=sizeof(WNDCLASSEX);
	wndclassex.style=CS_HREDRAW | CS_VREDRAW;
	wndclassex.cbClsExtra=0;
	wndclassex.cbWndExtra=0;
	wndclassex.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);
	wndclassex.hInstance=hInstance;
	wndclassex.lpfnWndProc=WndProc;
	wndclassex.lpszClassName=szClassName;
	wndclassex.lpszMenuName=NULL;
	wndclassex.hIcon=LoadIcon(NULL,IDI_APPLICATION);
	wndclassex.hIconSm=LoadIcon(NULL,IDI_APPLICATION);
	wndclassex.hCursor=LoadCursor(NULL,IDC_ARROW);

	RegisterClassEx(&wndclassex);

	hwnd=CreateWindow(szClassName,szClassName,WS_OVERLAPPEDWINDOW,
					100,100,WIN_WIDTH,WIN_HEIGHT,
					NULL,NULL,hInstance,NULL);

	ghwnd=hwnd;

	ShowWindow(hwnd,iCmdShow);
	SetForegroundWindow(hwnd);
	SetFocus(hwnd);

	initialize();

	while(bDone==false){
		if(PeekMessage(&msg,NULL,0,0,PM_REMOVE)){
			if(msg.message==WM_QUIT){
				bDone=true;
			}else{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}else{
			if(gbActiveWindow==true){
				if(gbEscapeKeyIsPressed==true){
					bDone=true;
				}
			}
			display();
		}
	}

	uninitialize();
	return((int)msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hwnd,UINT iMsg,WPARAM wParam,LPARAM lParam){
	void resize(int,int);
	void ToggleFullscreen(void);
	void uninitialize(void);
	switch(iMsg){
		case WM_ACTIVATE:
			if(HIWORD(wParam)==0){
				gbActiveWindow=true;
			}else{
				gbActiveWindow=false;
			}
			break;
		case WM_SIZE:
			resize(LOWORD(lParam),HIWORD(lParam));
			break;
		case WM_KEYDOWN:
			switch(LOWORD(wParam)){
				case VK_ESCAPE:
					if(gbEscapeKeyIsPressed==false){
						gbEscapeKeyIsPressed=true;
					}
					break;
				case 0x46:
					if(gbFullscreen==false){
						ToggleFullscreen();
						gbFullscreen=true;
					}else{
						ToggleFullscreen();
						gbFullscreen=false;
					}
					break;
				default:
					break;
			}
			break;
		case WM_CLOSE:
			uninitialize();
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			break;
	}
	return (DefWindowProc(hwnd,iMsg,wParam,lParam));
}


void ToggleFullscreen(void){
	MONITORINFO mi;
	bool bGetWindowPlacement=false;
	bool bGetMonitorInfo=false;
	if(gbFullscreen==false){
		dwStyle=GetWindowLong(ghwnd,GWL_STYLE);
		if(dwStyle & WS_OVERLAPPEDWINDOW){
			mi={sizeof(MONITORINFO)};
			bGetWindowPlacement=GetWindowPlacement(ghwnd,&wpPrev);
			bGetMonitorInfo=GetMonitorInfo(MonitorFromWindow(ghwnd,MONITORINFOF_PRIMARY),&mi);
			if(bGetWindowPlacement && bGetMonitorInfo){
				SetWindowLong(ghwnd,GWL_STYLE,dwStyle & ~WS_OVERLAPPEDWINDOW);
				SetWindowPos(ghwnd,HWND_TOP,mi.rcMonitor.left,mi.rcMonitor.top,
							mi.rcMonitor.right-mi.rcMonitor.left,
							mi.rcMonitor.bottom-mi.rcMonitor.top,
							SWP_NOZORDER | SWP_FRAMECHANGED);
			}
		}	
		ShowCursor(FALSE);	
	}else{
		SetWindowLong(ghwnd,GWL_STYLE,dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(ghwnd,&wpPrev);
		SetWindowPos(ghwnd,HWND_TOP,0,0,0,0,
					SWP_NOMOVE | SWP_NOSIZE |
					SWP_NOOWNERZORDER | SWP_NOZORDER |
					SWP_FRAMECHANGED);
	}	ShowCursor(TRUE);
}

void initialize(void){
	PIXELFORMATDESCRIPTOR pfd;
	int iPixelFormatIndex;

	void resize(int,int);
	void uninitialize(void);
	void createPoints(GLint,GLint);

	ZeroMemory(&pfd,sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize=sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion=1;
	pfd.dwFlags=PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType=PFD_TYPE_RGBA;
	pfd.cColorBits=32;
	pfd.cRedBits=8;
	pfd.cGreenBits=8;
	pfd.cBlueBits=8;
	pfd.cAlphaBits=8;
	pfd.cDepthBits=32;

	ghdc=GetDC(ghwnd);
	iPixelFormatIndex=ChoosePixelFormat(ghdc,&pfd);
	if(iPixelFormatIndex==0){
		ReleaseDC(ghwnd,ghdc);
		ghdc=NULL;
	}

	if(SetPixelFormat(ghdc,iPixelFormatIndex,&pfd)==false){
		ReleaseDC(ghwnd,ghdc);
		ghdc=NULL;
	}

	ghrc=wglCreateContext(ghdc);
	if(ghrc==NULL){
		ReleaseDC(ghwnd,ghdc);
		ghdc=NULL;
	}

	if(wglMakeCurrent(ghdc,ghrc)==false){
		wglDeleteContext(ghrc);
		ghrc=NULL;
		ReleaseDC(ghwnd,ghdc);
		ghdc=NULL;
	}

	GLenum glew_error=glewInit();
	if(glew_error!=GLEW_OK){
		wglDeleteContext(ghrc);
		ghrc=NULL;
		ReleaseDC(ghwnd,ghdc);
		ghdc=NULL;
	}

	createPoints(100,100);

	gVertexShaderObject=glCreateShader(GL_VERTEX_SHADER);

	const GLchar *vertexShaderSourceCode=
		"#version 430"\
		"\n"\
		"uniform float Time;"\
		"uniform vec4 Background;"\
		"uniform mat4 MVPMatrix;"
		"in vec4 MCVertex;"\
		"in vec4 MColor;"\
		"in vec3 Velocity;"\
		"out vec3 ChangeVelocity;"\
		"in float StartTime;"\
		"out vec4 Color;"\
		"void main(void)"\
		"{"\
			"vec4 vert;"\
			"float t=Time-StartTime;"\
			"if(t >= 0.0)"\
			"{"\
				"vert=MCVertex + vec4(Velocity*t,0.0);"\
				"vert.z=9.8*t*t;"\
				"Color=MColor;"\
				"ChangeVelocity=Velocity;"\
			"}"\
			"else"\
			"{"\
				"vert=MCVertex;"\
				"Color=Background;"\
				"ChangeVelocity=Velocity;"\
			"}"\
			"gl_Position=MVPMatrix*vert;"\
		"}";



	glShaderSource(gVertexShaderObject,1,
		(const GLchar **)&vertexShaderSourceCode,NULL);

	glCompileShader(gVertexShaderObject);

	GLint iInfoLogLength=0;
	GLint iShaderCompiledStatus=0;
	char *szInfoLog=NULL;
	glGetShaderiv(gVertexShaderObject,GL_COMPILE_STATUS,
					&iShaderCompiledStatus);
	if(iShaderCompiledStatus==GL_FALSE){
		glGetShaderiv(gVertexShaderObject,GL_INFO_LOG_LENGTH,
						&iInfoLogLength);
		if(iInfoLogLength>0){
			szInfoLog=(char *)malloc(iInfoLogLength);
			if(szInfoLog!=NULL){
				GLsizei written;
				glGetShaderInfoLog(gVertexShaderObject,iInfoLogLength,
									&written,szInfoLog);
				fprintf(gpFile,"Vertex Shader Compilation Log:%s\n",szInfoLog);
				free(szInfoLog);
				uninitialize();
				exit(EXIT_FAILURE);
			}
		}
	}

	gFragmentShaderObject=glCreateShader(GL_FRAGMENT_SHADER);
	const GLchar *fragmetShaderSourceCode=
		"#version 430" \
		"\n" \
		"in vec4 Color;" \
		"in vec3 ChangeVelocity;"\
		"out vec4 out_Color;"\
		"void main(void)" \
		"{" \
		"out_Color=Color;" \
		"}";

	glShaderSource(gFragmentShaderObject,1,
				(const GLchar**)&fragmetShaderSourceCode,NULL);

	glCompileShader(gFragmentShaderObject);
	glGetShaderiv(gFragmentShaderObject,GL_COMPILE_STATUS,
					&iShaderCompiledStatus);
	if(iShaderCompiledStatus==GL_FALSE){
		glGetShaderiv(gFragmentShaderObject,GL_INFO_LOG_LENGTH,
					&iInfoLogLength);
		if(iInfoLogLength>0){
			szInfoLog=(char*)malloc(iInfoLogLength);
			if(szInfoLog!=NULL){
				GLsizei written;
				glGetShaderInfoLog(gFragmentShaderObject,iInfoLogLength,
					&written,szInfoLog);
				fprintf(gpFile,"Fragment Shader Compilation Log:%s\n",szInfoLog);
				free(szInfoLog);
				uninitialize();
				exit(EXIT_FAILURE);
			}
		}
	}
	gShaderProgramObject=glCreateProgram();

	glAttachShader(gShaderProgramObject,gVertexShaderObject);
	glAttachShader(gShaderProgramObject,gFragmentShaderObject);

	glBindAttribLocation(gShaderProgramObject,SSK_VERTEX_ARRAY,"MCVertex");
	glBindAttribLocation(gShaderProgramObject,SSK_COLOR_ARRAY,"MColor");
	glBindAttribLocation(gShaderProgramObject,SSK_VELOCITY_ARRAY,"Velocity");
	glBindAttribLocation(gShaderProgramObject,SSK_START_TIME_ARRAY,"StartTime");

	glLinkProgram(gShaderProgramObject);

	GLint iShaderProgramLinkStatus=0;
	glGetProgramiv(gShaderProgramObject,GL_LINK_STATUS,
				&iShaderProgramLinkStatus);
	if(iShaderProgramLinkStatus==GL_FALSE){
		glGetProgramiv(gShaderProgramObject,GL_INFO_LOG_LENGTH,
						&iInfoLogLength);
		if(iInfoLogLength>0){
			szInfoLog=(char *)malloc(iInfoLogLength);
			if(szInfoLog!=NULL){
				GLsizei written;
				glGetProgramInfoLog(gShaderProgramObject,iInfoLogLength,&written,szInfoLog);
				fprintf(gpFile,"Shader Program Link Log : %s\n",szInfoLog);
				free(szInfoLog);
				uninitialize();
				exit(0);
			}
		}
	}


	gMVPUniform= glGetUniformLocation(gShaderProgramObject,"MVPMatrix");
	location = glGetUniformLocation(gShaderProgramObject, "Time");

	glPointSize(2.0f);
	glGenVertexArrays(1,&gVao);
	glBindVertexArray(gVao);
		
		glGenBuffers(1,&gVbo);
			glBindBuffer(GL_ARRAY_BUFFER,gVbo);
			glBufferData(GL_ARRAY_BUFFER,(arrayWidth*arrayHeight*3*sizeof(float)),
						verts,GL_STATIC_DRAW);
 			glVertexAttribPointer(SSK_VERTEX_ARRAY,3,GL_FLOAT,GL_FALSE,0,0);
			glEnableVertexAttribArray(SSK_VERTEX_ARRAY);	
		glBindBuffer(GL_ARRAY_BUFFER,0);
		
		glGenBuffers(1,&gVbo2);
			glBindBuffer(GL_ARRAY_BUFFER,gVbo2);
			glBufferData(GL_ARRAY_BUFFER,(arrayWidth*arrayHeight*3*sizeof(float)),
						colors,GL_STATIC_DRAW);
 			glVertexAttribPointer(SSK_COLOR_ARRAY,3,GL_FLOAT,GL_FALSE,0,0);
			glEnableVertexAttribArray(SSK_COLOR_ARRAY);
		glBindBuffer(GL_ARRAY_BUFFER,0);
 		
 		glGenBuffers(1,&gVbo3);
			glBindBuffer(GL_ARRAY_BUFFER,gVbo3);
			glBufferData(GL_ARRAY_BUFFER,(arrayWidth*arrayHeight*3*sizeof(float)),
						velocities,GL_STATIC_DRAW);
 			glVertexAttribPointer(SSK_VELOCITY_ARRAY, 3, GL_FLOAT,GL_FALSE,0,0);
			glEnableVertexAttribArray(SSK_VELOCITY_ARRAY);
		glBindBuffer(GL_ARRAY_BUFFER,0);
		
		glGenBuffers(1,&gVbo4);
			glBindBuffer(GL_ARRAY_BUFFER,gVbo4);
			glBufferData(GL_ARRAY_BUFFER,(arrayWidth*arrayHeight*sizeof(float)),
						startTimes,GL_STATIC_DRAW);
	 		glVertexAttribPointer(SSK_START_TIME_ARRAY, 1, GL_FLOAT,GL_FALSE,0,0);
			glEnableVertexAttribArray(SSK_START_TIME_ARRAY);
		glBindBuffer(GL_ARRAY_BUFFER,0);
	glBindVertexArray(0);

	glShadeModel(GL_SMOOTH);

	glClearColor(0.0f,0.0f,0.0f,1.0f);
	gPerspectiveProjectionMatrix=vmath::mat4::identity();
	resize(WIN_WIDTH,WIN_HEIGHT);


}

void display(void){
	void drawPoints(void);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
					GL_STENCIL_BUFFER_BIT);
	glUseProgram(gShaderProgramObject);

	vmath::mat4 modelViewMatrix=vmath::mat4::identity();
	vmath::mat4 modelViewProjectionMatrix=vmath::mat4::identity();
	vmath::mat4 rotationMatrix=vmath::mat4::identity();

	modelViewMatrix=vmath::translate(0.0f,0.0f,-50.0f);
	modelViewProjectionMatrix=gPerspectiveProjectionMatrix*modelViewMatrix;
	glUniformMatrix4fv(gMVPUniform,1,GL_FALSE,modelViewProjectionMatrix);
	glUniform1f(location, ParticleTime);
		
	glBindVertexArray(gVao);
		glDrawArrays(GL_POINTS, 0, arrayWidth*arrayHeight);
	glBindVertexArray(0);

	ParticleTime = ParticleTime + 0.001f;	

	glUseProgram(0);
	SwapBuffers(ghdc);
}

void resize(int width, int height){
	if(height==0){
		height=1;
	}

	glViewport(0,0,(GLsizei)width,(GLsizei)height);

	if(width<=height){
		gPerspectiveProjectionMatrix=vmath::perspective(45.0f,
														(GLfloat)height/(GLfloat)width,
														0.1f,100.0f);
	}else{
		gPerspectiveProjectionMatrix=vmath::perspective(45.0f,
														(GLfloat)width/(GLfloat)height,
														0.1f,100.0f);
	}
}

void uninitialize(void){
	if(gbFullscreen==true){
		dwStyle=GetWindowLong(ghwnd,GWL_STYLE);
		SetWindowLong(ghwnd,GWL_STYLE,dwStyle| WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(ghwnd,&wpPrev);
		SetWindowPos(ghwnd,HWND_TOP,0,0,0,0,
					SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
					SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		ShowCursor(TRUE);
	}
	if(gVao){
		glDeleteVertexArrays(1,&gVao);
		gVao=0;
	}

	if(gVbo){
		glDeleteBuffers(1,&gVbo);
		gVbo=0;
	}

	glDetachShader(gShaderProgramObject,gVertexShaderObject);
	glDetachShader(gShaderProgramObject,gFragmentShaderObject);

	glDeleteShader(gVertexShaderObject);
	gVertexShaderObject=0;
	glDeleteShader(gFragmentShaderObject);
	gFragmentShaderObject=0;

	glDeleteProgram(gShaderProgramObject);
	gShaderProgramObject=0;

	glUseProgram(0);


	wglMakeCurrent(NULL,NULL);
	wglDeleteContext(ghrc);
	ghrc=NULL;

	ReleaseDC(ghwnd,ghdc);
	ghdc=NULL;

	if(gpFile){
		fprintf(gpFile,"Log File is Successfully Closed.\n");
		fclose(gpFile);
		gpFile=NULL;
	}
}


void createPoints(GLint w, GLint h){
	GLfloat *vptr, *cptr, *velptr, *stptr;
	GLfloat i, j;
	if(verts != NULL){
 		free(verts);
 	}
 	GLfloat r=0.1f;

 	verts = (GLfloat *) malloc(w * h * 3 * sizeof(float));
 	colors = (GLfloat *) malloc(w * h * 3 * sizeof(float));
 	velocities = (GLfloat *) malloc(w * h * 3 * sizeof(float));
 	startTimes = (GLfloat *) malloc(w * h * sizeof(float));
 	float move=0.1f;
 	vptr = verts;
 	cptr = colors;
 	velptr = velocities;
 	stptr = startTimes;

 	for (i = 0.5 / w - 0.5; i < 0.5; i = i + 1.0/w)
 	{
 		for (j = 0.5 / h - 0.5; j < 0.5; j = j + 1.0/h){
 			
			*vptr       = i+ cos(angle);
 			*(vptr + 1) = i+sin(angle);
 			*(vptr + 2) = 0;
 			vptr 		=vptr + 3;
/*
 			*cptr 		= ((float) rand() / RAND_MAX) * 0.5f + 0.5f;
 			*(cptr + 1) = ((float) rand() / RAND_MAX) * 0.5f + 0.5f;
 			*(cptr + 2) = ((float) rand() / RAND_MAX) * 0.5f + 0.5f;
 			cptr 		=cptr + 3;
*/
 			fprintf(gpFile, "value %f\n",j );
 			if(i<=0.20){
	 			*cptr 		=  1.0f;
 				*(cptr + 1) =  0.0f;
 				*(cptr + 2) =  0.0f;
	 			cptr 		=cptr + 3;
 			}else if(i>=0.500){
				*cptr 		=  0.0f;
 				*(cptr + 1) =  1.0f;
 				*(cptr + 2) =  0.0f;
 				cptr 		=cptr + 3;
			}else{
				*cptr 		=  1.0f;
 				*(cptr + 1) =  1.0f;
 				*(cptr + 2) =  1.0f;
 				cptr 		=cptr + 3;
			}

/* 			*cptr 		=  0.8f + 0.5f;
 			*(cptr + 1) =  0.8f + 0.5f;
 			*(cptr + 2) =  0.8f + 0.5f;
 			cptr 		=cptr + 3;
*/
 			*velptr 		= cos(angle) * 10.0f;
 			*(velptr + 1) 	= sin(angle) * 10.0f;
 			*(velptr + 2) 	= 0.0f;
 			velptr 			=velptr + 3; 			

 			*stptr  = ((float) rand() / RAND_MAX)*10.0;
 			stptr 	=stptr+1;
 		
 			angle=angle+180.0f;
 			//r=r+0.1f;		
 		}
	}
	arrayWidth = w;
	arrayHeight = h;
}

