#include "gl2widget.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <QFileDialog>
#include <QCoreApplication>
#include <QFrame>
#include <QDebug>
#include <cassert>

GL2Widget::GL2Widget(QWidget * parent)
        : QOpenGLWidget(parent),
          _updateTimer(nullptr),
          _cam(glm::vec3(25.0f, 50.0f, 25.0f),
               glm::vec3(0.0f, 0.0f, 0.0f),
               glm::vec3(0.0f, 1.0f, 0.0f)),
          _wire(false),
          _bbox_vbo_vertices(0),
          _bbox_ibo_elements(0),
          _texLoaded(false)
{
    timer.start();

    //_updateTimer = new QTimer(this);
    connect(&_updateTimer, SIGNAL(timeout()), this, SLOT(update()));
    _updateTimer.start(16);

    setFocusPolicy(Qt::StrongFocus);
}

GL2Widget::~GL2Widget()
{
    if(!_glSubMeshes.empty())
    {
        ClearData();
    }
    
    glDeleteBuffers(1, &_bbox_vbo_vertices);
    glDeleteBuffers(1, &_bbox_ibo_elements);
}

QSize GL2Widget::minimumSizeHint() const
{
    return QSize(100, 100);
}

QSize GL2Widget::sizeHint() const
{
    return QSize(640, 480);
}

void GL2Widget::loadMesh()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Mesh"),
                                                    ".",
                                                    tr("Mesh files (*.msh)"));

    if(fileName.isEmpty())
            return;

    QByteArray ba = fileName.toUtf8();

    _mainMesh = Mesh();
    ClearData();

    if(_mainMesh.LoadFromMsh(ba.data()))
    {
        UploadData();

        int num_tri = 0;
        for(auto & m : _mainMesh._meshes)
        {
            num_tri += m._indices.size()/3;
        }

        emit numTriChanged(num_tri);
        emit anmPresent(!_mainMesh._meshes[0]._wght_inds.empty());
        emit anmLoaded(0);
        emit stateBBoxCheck(false);
    }
}

void GL2Widget::loadAnimation()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Mesh"),
                                                    ".",
                                                    tr("Animation files (*.anm)"));

    if(fileName.isEmpty())
            return;

    if(_mainMesh.LoadFromAnm(fileName.toUtf8().data()))
    {
        emit anmLoaded(_mainMesh._anims[0].frames.size());
    }
}

void GL2Widget::loadTexture()
{
    _texLoaded = false;

    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Mesh"),
                                                    ".",
                                                    tr("Images (*.tga *.bmp)"));

    if(fileName.isEmpty())
            return;

    if(!_mainMesh.LoadTexture(fileName.toUtf8().data()))
    {
        qDebug() << "Fail to load texture";
        return;
    }

    _texLoaded = true;
    if(isMshLoaded())
    {
        for(auto & gl_msh : _glSubMeshes)
        {
            glGenTextures(1, &gl_msh._tex);
            glBindTexture(GL_TEXTURE_2D, gl_msh._tex);

            glTexImage2D(GL_TEXTURE_2D, 0, _mainMesh._texData.type == ImageData::PixelType::pt_rgb ? 3 : 4,
                         _mainMesh._texData.width, _mainMesh._texData.height, 0,
                         _mainMesh._texData.type == ImageData::PixelType::pt_rgb ? GL_RGB : GL_RGBA,
                         GL_UNSIGNED_BYTE, _mainMesh._texData.data.get());

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
}

void GL2Widget::drawBBox(int state)
{
    _mainMesh.DrawBBox(state == Qt::Checked);
}

void GL2Widget::initializeGL()
{
    // connections
    initializeOpenGLFunctions();
    glViewport(0, 0, size().width(), size().height());

    //Shading states
    glShadeModel(GL_SMOOTH);
    glClearColor(0.0f, 0.1f, 0.4f, 0.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat mat_shininess[] = { 50.0 };
    GLfloat light_position[] = { 5.0, 1.0, 5.0, 0.0 };
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    //Depth states
    glClearDepth(1.0);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    glEnable( GL_TEXTURE_2D );

     auto projectionMatrix = glm::perspective(45.0f,                       // 45° Field of View
                                         static_cast<float>(size().width())
                                         / static_cast<float>(size().height()),
                                         0.1f, 100.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(projectionMatrix));

    glMatrixMode(GL_MODELVIEW);
    auto viewMatrix = _cam.GetViewMatrix();
    glLoadMatrixf(glm::value_ptr(viewMatrix));
}

void GL2Widget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    RenderMesh();
}

void GL2Widget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    auto projectionMatrix = glm::perspective(45.0f,
                                         static_cast<float>(w) / static_cast<float>(h),
                                         0.1f, 100.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(projectionMatrix));
}

void GL2Widget::UploadData()
{
    // upload BBox vbo once
    if(_bbox_vbo_vertices == 0)
    {
        float vertices[] = {
                            -0.5f, -0.5f, -0.5f, 1.0f,
                             0.5f, -0.5f, -0.5f, 1.0f,
                             0.5f,  0.5f, -0.5f, 1.0f,
                            -0.5f,  0.5f, -0.5f, 1.0f,
                            -0.5f, -0.5f,  0.5f, 1.0f,
                             0.5f, -0.5f,  0.5f, 1.0f,
                             0.5f,  0.5f,  0.5f, 1.0f,
                            -0.5f,  0.5f,  0.5f, 1.0f,
                            };

        unsigned short elements[] = {
                            0, 1, 2, 3,
                            4, 5, 6, 7,
                            0, 4, 1, 5,
                            2, 6, 3, 7
                            };

        glGenBuffers(1, &_bbox_vbo_vertices);
        glGenBuffers(1, &_bbox_ibo_elements);

        glBindBuffer(GL_ARRAY_BUFFER, _bbox_vbo_vertices);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _bbox_ibo_elements);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    if(!_glSubMeshes.empty())
    {
        ClearData();
    }

    _glSubMeshes.clear();
    _glSubMeshes.resize(_mainMesh._meshes.size());

    for(unsigned int i = 0; i < _mainMesh._meshes.size(); i++)
    {
        auto & msh = _mainMesh._meshes[i];

        assert(msh._positions.size() > 0);
        assert(msh._uvs.size() > 0);
        assert(msh._normals.size() > 0);
        assert(msh._indices.size() > 0);

        glGenBuffers(1, &_glSubMeshes[i]._vertexbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, _glSubMeshes[i]._vertexbuffer);
        glBufferData(GL_ARRAY_BUFFER, msh._positions.size() * sizeof(glm::vec3), &msh._positions[0], GL_STREAM_DRAW);

        glGenBuffers(1, &_glSubMeshes[i]._uvbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, _glSubMeshes[i]._uvbuffer);
        glBufferData(GL_ARRAY_BUFFER, msh._uvs[0].size() * sizeof(glm::vec2), msh._uvs[0].data(), GL_STATIC_DRAW);

        glGenBuffers(1, &_glSubMeshes[i]._normalbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, _glSubMeshes[i]._normalbuffer);
        glBufferData(GL_ARRAY_BUFFER, msh._normals.size() * sizeof(glm::vec3), &msh._normals[0], GL_STREAM_DRAW);

        // Generate a buffer for the indices as well
        glGenBuffers(1, &_glSubMeshes[i]._elementbuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _glSubMeshes[i]._elementbuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, msh._indices.size() * sizeof(unsigned int), &msh._indices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        if(_texLoaded)
        {
            glGenTextures(1, &_glSubMeshes[i]._tex);
            glBindTexture(GL_TEXTURE_2D, _glSubMeshes[i]._tex);

            glTexImage2D(GL_TEXTURE_2D, 0, _mainMesh._texData.type == ImageData::PixelType::pt_rgb ? 3 : 4,
                         _mainMesh._texData.width, _mainMesh._texData.height, 0,
                         _mainMesh._texData.type == ImageData::PixelType::pt_rgb ? GL_RGB : GL_RGBA,
                         GL_UNSIGNED_BYTE, _mainMesh._texData.data.get());

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
}

void GL2Widget::ClearData()
{
    for(auto & gl_msh : _glSubMeshes)
    {
        glDeleteBuffers(1, &gl_msh._vertexbuffer);
        glDeleteBuffers(1, &gl_msh._normalbuffer);
        glDeleteBuffers(1, &gl_msh._uvbuffer);
        glDeleteBuffers(1, &gl_msh._elementbuffer);
        if(_texLoaded)
            glDeleteTextures(1, &gl_msh._tex);
    }
}

void GL2Widget::RenderMesh()
{
    if(!isMshLoaded())
        return;
    float        frameDelta(0.0f);
    unsigned int prevFrame = 0;
    unsigned int nextFrame = 0;

    if(isAnmLoaded())
    {
        double       controlTime = _mainMesh._controller.GetControlTime(elapsed());
        prevFrame = glm::floor(controlTime * _mainMesh._anims[0].frameRate);
        nextFrame = prevFrame + 1;
        if(prevFrame == _mainMesh._anims[0].frames.size() - 1)
            nextFrame = 0;

        frameDelta = controlTime * _mainMesh._anims[0].frameRate - prevFrame;

        Mesh::AnimSequence::JointNode tr;
        for(unsigned int i = 0; i < _mainMesh._anims[0].frames[0].rot.size(); i++)
        {
            tr.rot.push_back(glm::normalize(glm::slerp(_mainMesh._anims[0].frames[prevFrame].rot[i],
                                                       _mainMesh._anims[0].frames[nextFrame].rot[i],
                                                       frameDelta)));
            tr.trans.push_back(glm::mix(_mainMesh._anims[0].frames[prevFrame].trans[i],
                                        _mainMesh._anims[0].frames[nextFrame].trans[i],
                                        frameDelta));

        }

        for(unsigned int i = 0; i < _mainMesh._meshes.size(); i++)
        {
            auto & sub_msh = _mainMesh._meshes[i];
            std::vector<glm::vec3> curPosVec;
            std::vector<glm::vec3> curNorVec;

            for(unsigned int n = 0; n < sub_msh._positions.size(); n++)
            {
                glm::mat4 matTr(0.0f);
                for(unsigned int j = 0; j < sub_msh._wght_inds[n].second
                                            - sub_msh._wght_inds[n].first; j++)
                {
                    glm::mat4 mt = glm::mat4_cast(tr.rot[sub_msh._weights[sub_msh._wght_inds[n].first + j].jnt_index - 1]);
                    mt = glm::column(mt, 3,
                            glm::vec4(tr.trans[sub_msh._weights[sub_msh._wght_inds[n].first + j].jnt_index - 1], 1.0f));

                    matTr += sub_msh._weights[sub_msh._wght_inds[n].first + j].w * mt;
                }

                glm::vec4 cpos = matTr * glm::vec4(sub_msh._positions[n], 1.0);
                glm::vec3 norm = glm::mat3(matTr) * sub_msh._normals[n];

                curPosVec.push_back(glm::vec3(cpos));
                curNorVec.push_back(norm);
            }

            glBindBuffer(GL_ARRAY_BUFFER_ARB, _glSubMeshes[i]._vertexbuffer);
            glBufferSubData(GL_ARRAY_BUFFER_ARB, 0, curPosVec.size() * 3 * sizeof(float), &curPosVec[0]);

            glBindBuffer(GL_ARRAY_BUFFER_ARB, _glSubMeshes[i]._normalbuffer);
            glBufferSubData(GL_ARRAY_BUFFER_ARB, 0, curNorVec.size() * 3 * sizeof(float), &curNorVec[0]);
        }
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMultMatrixf(glm::value_ptr(_cam.GetViewMatrix()));
    glPushMatrix();
    glMultMatrixf(glm::value_ptr(_mainMesh._modelMatrix));

    glPolygonMode( GL_FRONT_AND_BACK, _wire ? GL_LINE : GL_FILL );
    for(unsigned int i = 0; i < _mainMesh._meshes.size(); i++)
    {
        assert(_glSubMeshes[i]._vertexbuffer > 0);
        assert(_glSubMeshes[i]._normalbuffer > 0);
        assert(_glSubMeshes[i]._uvbuffer > 0);
        assert(_glSubMeshes[i]._elementbuffer > 0);

        glBindTexture(GL_TEXTURE_2D, _glSubMeshes[i]._tex);

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _glSubMeshes[i]._elementbuffer);

        glBindBuffer(GL_ARRAY_BUFFER, _glSubMeshes[i]._normalbuffer);
        glNormalPointer(GL_FLOAT, 0, (char*)NULL);
        glBindBuffer(GL_ARRAY_BUFFER, _glSubMeshes[i]._uvbuffer);
        glTexCoordPointer(2, GL_FLOAT, 0, (char*)NULL);
        glBindBuffer(GL_ARRAY_BUFFER, _glSubMeshes[i]._vertexbuffer);
        glVertexPointer(3, GL_FLOAT, 0, (char*)NULL);

        glDrawElements(GL_TRIANGLES, _mainMesh._meshes[i]._indices.size(), GL_UNSIGNED_INT, (char*)NULL);

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    glPopMatrix();

    if(_mainMesh.isDrawBBox())
    {
        AABB box;
        if(isAnmLoaded())
        {
            glm::vec3 min = glm::mix(_mainMesh._anims[0].frames[prevFrame].bbox.min(),
                                     _mainMesh._anims[0].frames[nextFrame].bbox.min(),
                                     frameDelta);
            glm::vec3 max = glm::mix(_mainMesh._anims[0].frames[prevFrame].bbox.max(),
                                     _mainMesh._anims[0].frames[nextFrame].bbox.max(),
                                     frameDelta);
            box = AABB(min, max);
        }
        else
            box = _mainMesh._base_bbox;

        box.transform(_mainMesh._modelMatrix);

        glm::vec3 size = box.max() - box.min();
        glm::vec3 center = (box.min() + box.max())/2.0f;
        glm::mat4 transform =  glm::translate(glm::mat4(1), center) * glm::scale(glm::mat4(1), size);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glMultMatrixf(glm::value_ptr(transform));

        glColor3f(1.0f, 0.0f, 0.0f);
        glDisable(GL_LIGHTING);

        glBindBuffer(GL_ARRAY_BUFFER, _bbox_vbo_vertices);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(
            4,                  // number of elements per vertex, here (x,y,z,w));
            GL_FLOAT,           // the type of each element
            0,                  // no extra data between each position
            0                   // offset of first element
            );
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _bbox_ibo_elements);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1, 0);
        glLineWidth(2);

        glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_SHORT, 0);
        glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_SHORT, (GLvoid*)(4*sizeof(GLushort)));
        glDrawElements(GL_LINES, 8, GL_UNSIGNED_SHORT, (GLvoid*)(8*sizeof(GLushort)));

        glDisable(GL_POLYGON_OFFSET_FILL);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDisableClientState(GL_VERTEX_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glPopMatrix();
        glLineWidth(1);
        glEnable(GL_LIGHTING);
    }
}

void GL2Widget::keyPressEvent(QKeyEvent * event)
{
    switch (event->key()) {
        case Qt::Key_Left:
            _mainMesh.RotateMesh(glm::vec3(0, 0, glm::radians(-5.0f)));
            break;
        case Qt::Key_Right:
            _mainMesh.RotateMesh(glm::vec3(0, 0, glm::radians(5.0f)));
            break;
        case Qt::Key_Down:
            _mainMesh.RotateMesh(glm::vec3(glm::radians(5.0f), 0, 0));
            break;
        case Qt::Key_Up:
            _mainMesh.RotateMesh(glm::vec3(glm::radians(-5.0f), 0, 0));
            break;
        case Qt::Key_W:
            _wire = !_wire;
            break;
        case Qt::Key_Escape:
            QCoreApplication::quit();
            break;
        default:
            QWidget::keyPressEvent(event);
        }
}

void GL2Widget::mousePressEvent(QMouseEvent *event)
{
    _lastPos = event->pos();
}

void GL2Widget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - _lastPos.x();
    int dy = event->y() - _lastPos.y();

    if (event->buttons() & Qt::LeftButton)
    {
        float x = 2.0f * dy;
        float y = 2.0f * dx;
        _mainMesh.RotateMesh(glm::vec3(glm::radians(x), glm::radians(y), 0.0f));
    }
    else if (event->buttons() & Qt::RightButton)
    {
        float x = 2.0f * dy;
        float z = 2.0f * dx;
        _mainMesh.RotateMesh(glm::vec3(glm::radians(x), 0.0f, glm::radians(z)));
    }
    _lastPos = event->pos();
}

void GL2Widget::wheelEvent(QWheelEvent *event)
{
    QPoint numDegrees = event->angleDelta() / 8;
    QPoint numSteps = numDegrees / 15;

    _cam.MoveForward(numSteps.y() * 2.0f);

    event->accept();
}
