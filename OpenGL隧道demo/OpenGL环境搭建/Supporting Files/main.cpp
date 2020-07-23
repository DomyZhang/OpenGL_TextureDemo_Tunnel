
#include "GLTools.h"
#include "GLShaderManager.h"
#include "GLFrustum.h"
#include "GLBatch.h"
#include "GLFrame.h"
#include "GLMatrixStack.h"
#include "GLGeometryTransform.h"

#ifdef __APPLE__
#include <glut/glut.h>
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>
#endif

GLShaderManager shaderManager;// 着色器管理器
GLMatrixStack       modelViewMatix;// 模型视图矩阵
GLMatrixStack       projectionMatrix;// 投影矩阵
GLFrustum           viewFrustum;// 视景体 透视投影 - GLFrustum类
GLGeometryTransform transformPipeline;// 几何变换管道

//// 设置角色帧，作为相机
//GLFrame  cameraFrame;// 观察者位置
//GLTriangleBatch     torusBatch;

// 4个批次容器类
GLBatch             floorBatch;//地面
GLBatch             ceilingBatch;//天花板
GLBatch             leftWallBatch;//左墙面
GLBatch             rightWallBatch;//右墙面


// 深度初始值，-65。
GLfloat viewZ = -65.0f;

// 纹理标识符号
#define TEXTURE_BRICK   0 //墙面
#define TEXTURE_FLOOR   1 //地板
#define TEXTURE_CEILING 2 //纹理天花板

#define TEXTURE_COUNT   3 //纹理个数

GLuint  textures[TEXTURE_COUNT];//纹理标记数组

// 文件 tag 数组
const char *textureFiles[TEXTURE_COUNT] = { "brick.tga", "floor.tga", "ceiling.tga" };


// 右键菜单栏选项 不同过滤方式
void ProcessMenu(int value) {
    
    GLint iLoop;
    
    for(iLoop = 0; iLoop < TEXTURE_COUNT; iLoop++) {
        
        /* 绑定纹理 glBindTexture
         参数1：GL_TEXTURE_2D
         参数2：需要绑定的纹理对象
         */
        glBindTexture(GL_TEXTURE_2D, textures[iLoop]);
        
        /* 配置纹理参数 glTexParameteri
         参数1：纹理模式
         参数2：纹理参数
         参数3：特定纹理参数
         */
        switch(value) {
            case 0:
                // GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER(缩小过滤器)，GL_NEAREST（最邻近过滤）
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                break;
                
            case 1:
                // GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER(缩小过滤器)，GL_LINEAR（线性过滤）
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                break;
                
            case 2:
                // GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER(缩小过滤器)，GL_NEAREST_MIPMAP_NEAREST（选择最邻近的Mip层，并执行最邻近过滤）
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
                break;
                
            case 3:
                // GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER(缩小过滤器)，GL_NEAREST_MIPMAP_LINEAR（在Mip层之间执行线性插补，并执行最邻近过滤）
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
                break;
                
            case 4:
                // GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER(缩小过滤器)，GL_NEAREST_MIPMAP_LINEAR（选择最邻近Mip层，并执行线性过滤）
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
                break;
                
            case 5:
                // GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER(缩小过滤器)，GL_LINEAR_MIPMAP_LINEAR（在Mip层之间执行线性插补，并执行线性过滤，又称为三线性过滤）
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                break;
                
            case 6:
                
                // 设置各向异性过滤
                GLfloat fLargest;
                // 获取各向异性过滤的最大数量
                glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
                // 设置纹理参数(各向异性采样)
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
                break;
                
            case 7:
                // 设置各向同性过滤，数量为1.0表示(各向同性采样)
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
                break;
                
        }
    }
    
    // 重绘
    glutPostRedisplay();
}

// 键盘上下左右 事件
void SpecialKeys(int key, int x, int  y) {
    
    if(key == GLUT_KEY_UP)
        //移动的是深度值，Z
        viewZ += 0.5f;
    
    if(key == GLUT_KEY_DOWN)
        viewZ -= 0.5f;
    
    // 更新窗口，即可回调到RenderScene函数里
    glutPostRedisplay();
}

// 初始化设置
void SetupRC() {
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);// 黑色
    shaderManager.InitializeStockShaders();
    
    
    GLbyte *pBytes;
    GLint iWidth, iHeight, iComponents;
    GLenum eFormat;
    
    // 生成纹理标记
    /* 分配纹理对象 glGenTextures
     参数1：纹理对象的数量
     参数2：纹理对象标识数组
     */
    glGenTextures(TEXTURE_COUNT, textures);
    
    
    // 设置纹理
    for (GLint i = 0; i < TEXTURE_COUNT; i++) {
        
        // 绑定纹理对象
        /*绑定纹理对象 glBindTexture
         参数1：纹理模式，GL_TEXTURE_1D,GL_TEXTURE_2D,GL_TEXTURE_3D
         参数2：需要绑定的纹理对象id
         */
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        
        
        // 加载 读取 tga 文件数据
        /* 加载 tga 文件
         参数1：纹理文件名称
         参数2：文件宽度变量地址
         参数3：文件高度变量地址
         参数4：文件组件变量地址
         参数5：文件格式变量地址
         
         返回值：pBytes，指向图像数据的指针
         */
        pBytes = gltReadTGABits(textureFiles[i], &iWidth, &iHeight, &iComponents, &eFormat);
        
        
        // 设置纹理参数
        // 设置放大缩小 过滤方式 GL_TEXTURE_MAG_FILTER（放大过滤器,GL_NEAREST(最邻近过滤)；GL_TEXTURE_MIN_FILTER（缩小过滤器,GL_NEAREST(最邻近过滤)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        
        // 设置环绕方式
        // GL_TEXTURE_WRAP_S(2D:s、t轴环绕),GL_CLAMP_TO_EDGE --> 环绕模式强制对范围之外的纹理坐标沿着合法的纹理单元的最后一行或一列进行采样
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        
        // 载入纹理
        /* 载入纹理 glTexImage2D
         参数1：纹理维度，GL_TEXTURE_2D
         参数2：mip贴图层次
         参数3：纹理单元存储的颜色成分（从读取像素图中获得）
         参数4：加载纹理宽度
         参数5：加载纹理的高度
         参数6：为纹理贴图指定一个边界宽度 0
         参数7、8：像素数据的数据类型, GL_UNSIGNED_BYTE无符号整型
         参数9：指向纹理图像数据的指针
         */
        glTexImage2D(GL_TEXTURE_2D, 0, iComponents, iWidth, iHeight, 0, eFormat, GL_UNSIGNED_BYTE, pBytes);
        //        glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
        
        
        //  生成纹理对象 glGenerateMipmap
        /* 为纹理对象生成一组完整的 mipmap
         参数1：纹理维度，GL_TEXTURE_1D,GL_TEXTURE_2D,GL_TEXTURE_2D
         */
        glGenerateMipmap(GL_TEXTURE_2D);
        
        // 释放原始纹理数据，不再需要纹理原始数据了
        free(pBytes);
    }
    
    
    // 设置隧道的上下左右4个面
    GLfloat z;// 深度，隧道的深度
    
    // 地板 -- 图 ‘地板’
    /*
     GLTools库中的容器类，GBatch -->  void GLBatch::Begin(GLenum primitive,GLuint nVerts,GLuint nTextureUnits = 0);
     参数1：图元枚举值
     参数2：顶点数
     参数3：纹理数 --> 1组或者2组纹理坐标
     */
    floorBatch.Begin(GL_TRIANGLE_STRIP, 28, 1);
    for(z = 60.0f; z >= 0.0f; z -=10.0f) {
        
        // 纹理坐标与顶点的对应
        floorBatch.MultiTexCoord2f(0, 0.0f, 0.0f);// 纹理坐标
        floorBatch.Vertex3f(-10.0f, -10.0f, z);// 顶点
        
        floorBatch.MultiTexCoord2f(0, 1.0f, 0.0f);
        floorBatch.Vertex3f(10.0f, -10.0f, z);
        
        floorBatch.MultiTexCoord2f(0, 0.0f, 1.0f);
        floorBatch.Vertex3f(-10.0f, -10.0f, z - 10.0f);
        
        floorBatch.MultiTexCoord2f(0, 1.0f, 1.0f);
        floorBatch.Vertex3f(10.0f, -10.0f, z - 10.0f);
    }
    floorBatch.End();
    
    // 天花板 -- 图 ‘天花板’
    ceilingBatch.Begin(GL_TRIANGLE_STRIP, 28, 1);
    for(z = 60.0f; z >= 0.0f; z -=10.0f) {
        
        ceilingBatch.MultiTexCoord2f(0, 0.0f, 1.0f);
        ceilingBatch.Vertex3f(-10.0f, 10.0f, z - 10.0f);
        
        ceilingBatch.MultiTexCoord2f(0, 1.0f, 1.0f);
        ceilingBatch.Vertex3f(10.0f, 10.0f, z - 10.0f);
        
        ceilingBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
        ceilingBatch.Vertex3f(-10.0f, 10.0f, z);
        
        ceilingBatch.MultiTexCoord2f(0, 1.0f, 0.0f);
        ceilingBatch.Vertex3f(10.0f, 10.0f, z);
    }
    ceilingBatch.End();
    
    // 左侧墙面 -- 图 ‘左墙’
    leftWallBatch.Begin(GL_TRIANGLE_STRIP, 28, 1);
    for(z = 60.0f; z >= 0.0f; z -=10.0f) {
        
        leftWallBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
        leftWallBatch.Vertex3f(-10.0f, -10.0f, z);
        
        leftWallBatch.MultiTexCoord2f(0, 0.0f, 1.0f);
        leftWallBatch.Vertex3f(-10.0f, 10.0f, z);
        
        leftWallBatch.MultiTexCoord2f(0, 1.0f, 0.0f);
        leftWallBatch.Vertex3f(-10.0f, -10.0f, z - 10.0f);
        
        leftWallBatch.MultiTexCoord2f(0, 1.0f, 1.0f);
        leftWallBatch.Vertex3f(-10.0f, 10.0f, z - 10.0f);
    }
    leftWallBatch.End();
    
    // 右侧墙面 -- 图 ‘右墙‘
    rightWallBatch.Begin(GL_TRIANGLE_STRIP, 28, 1);
    for(z = 60.0f; z >= 0.0f; z -=10.0f) {
        
        rightWallBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
        rightWallBatch.Vertex3f(10.0f, -10.0f, z);
        
        rightWallBatch.MultiTexCoord2f(0, 0.0f, 1.0f);
        rightWallBatch.Vertex3f(10.0f, 10.0f, z);
        
        rightWallBatch.MultiTexCoord2f(0, 1.0f, 0.0f);
        rightWallBatch.Vertex3f(10.0f, -10.0f, z - 10.0f);
        
        rightWallBatch.MultiTexCoord2f(0, 1.0f, 1.0f);
        rightWallBatch.Vertex3f(10.0f, 10.0f, z - 10.0f);
    }
    rightWallBatch.End();
}

// 渲染
void RenderScene(void) {
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    // 压栈
    modelViewMatix.PushMatrix();
    // z轴上平移
    modelViewMatix.Translate(0.0f, 0.0f, viewZ);
    
    // 纹理替换矩阵着色器
    /*
     参数1：GLT_SHADER_TEXTURE_REPLACE（着色器标签）
     参数2：模型视图投影矩阵
     参数3：纹理层
     */
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_REPLACE, transformPipeline.GetModelViewProjectionMatrix(),0);
    
    // 绑定纹理
    /*
     --> 为何又绑定一次？？？
     --> 为防止纹理 ID 绑定错误，状态机机制，在共同开发过程中，可能出现绑定的纹理出现变化
     */
    // 地板
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_FLOOR]);
    floorBatch.Draw();
    // 天花板
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_CEILING]);
    ceilingBatch.Draw();
    // 左右墙
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_BRICK]);
    leftWallBatch.Draw();
    rightWallBatch.Draw();
    
    // 出栈
    modelViewMatix.PopMatrix();
    
    // 交换缓冲区
    glutSwapBuffers();
}

// 视口  窗口大小改变时接受新的宽度和高度，其中0,0代表窗口中视口的左下角坐标，w，h代表像素
void ChangeSize(int w,int h) {
    // 防止h变为0
    if(h == 0)
        h = 1;
    
    // 设置视口窗口尺寸
    glViewport(0, 0, w, h);
    
    // setPerspective 函数的参数是一个从顶点方向看去的视场角度（用角度值表示）
    // 设置透视模式，初始化其透视矩阵
    viewFrustum.SetPerspective(80.0f, GLfloat(w)/GLfloat(h), 1.0f, 120.0f);
    
    //4.把 透视矩阵 加载到 透视矩阵对阵中
    projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
    
    //5.初始化渲染管线
    transformPipeline.SetMatrixStacks(modelViewMatix, projectionMatrix);
}

// 关闭渲染环境
void ShutdownRC(void) {
    // 删除纹理
    glDeleteTextures(TEXTURE_COUNT, textures);
}

int main(int argc,char* argv[]) {
    
    //设置当前工作目录，针对MAC OS X
    
    gltSetWorkingDirectory(argv[0]);
    
    //初始化GLUT库
    
    glutInit(&argc, argv);
    /* 初始化双缓冲窗口，其中标志GLUT_DOUBLE、GLUT_RGBA、GLUT_DEPTH、GLUT_STENCIL分别指
     双缓冲窗口、RGBA颜色模式、深度测试、模板缓冲区
     */
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    
    //GLUT窗口大小，标题窗口
    glutInitWindowSize(800,600);
    glutCreateWindow("Tunnel - 隧道");
    
    //注册回调函数
    glutReshapeFunc(ChangeSize);
    glutDisplayFunc(RenderScene);
    
    
    glutSpecialFunc(SpecialKeys);// 注册键盘特殊键位(上下左右键) 处理点击事件
    
    // 创建右键 Menu
    // 添加菜单入口，改变过滤器
    glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("GL_NEAREST",0);
    glutAddMenuEntry("GL_LINEAR",1);
    glutAddMenuEntry("GL_NEAREST_MIPMAP_NEAREST",2);
    glutAddMenuEntry("GL_NEAREST_MIPMAP_LINEAR", 3);
    glutAddMenuEntry("GL_LINEAR_MIPMAP_NEAREST", 4);
    glutAddMenuEntry("GL_LINEAR_MIPMAP_LINEAR", 5);
    glutAddMenuEntry("Anisotropic Filter", 6);
    glutAddMenuEntry("Anisotropic Off", 7);
    
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    
    
    
    //驱动程序的初始化中没有出现任何问题。
    GLenum err = glewInit();
    if(GLEW_OK != err) {
        
        fprintf(stderr,"glew error:%s\n",glewGetErrorString(err));
        return 1;
    }
    
    //调用SetupRC
    SetupRC();
    
    glutMainLoop();
    
    // 启动循环，关闭纹理
    ShutdownRC();
    
    return 0;
}




