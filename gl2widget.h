#ifndef GL2WIDGET_H
#define GL2WIDGET_H

#include <QOpenGLWidget>
#include <QElapsedTimer>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QKeyEvent>
#include <glm/glm.hpp>
#include "camera.h"
#include "Mesh.h"

class GL2Widget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    GL2Widget(QWidget *parent = nullptr);
    ~GL2Widget() override;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    double elapsed() const { return timer.elapsed()/1000.0; }
    bool   isMshLoaded() const { return _mainMesh._meshes.size() > 0; }
    bool   isAnmLoaded() const { return _mainMesh._anims.size() > 0; }

public slots:
    void loadMesh();
    void loadAnimation();
    void loadTexture();
    void drawBBox(int state);

signals:
    void numTriChanged(int numTri);
    void anmLoaded(int numFrames);
    void anmPresent(bool val);
    void stateBBoxCheck(bool val);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

    void RenderMesh();
    void UploadData();
    void ClearData();

private:
    QElapsedTimer timer;
    QTimer _updateTimer;

    Camera     _cam;

    bool      _wire;

    QPoint  _lastPos;

    struct GLSubMesh
    {
        unsigned int  _vertexbuffer;
        unsigned int  _normalbuffer;
        unsigned int  _uvbuffer;
        unsigned int  _elementbuffer;
        unsigned int  _tex;

        GLSubMesh() : _vertexbuffer(0),
                      _normalbuffer(0),
                      _uvbuffer(0),
                      _elementbuffer(0),
                      _tex(0) {}

    };

    unsigned int  _bbox_vbo_vertices;
    unsigned int  _bbox_ibo_elements;

    Mesh                   _mainMesh;
    bool                   _texLoaded;
    std::vector<GLSubMesh> _glSubMeshes;
};

#endif // GL2WIDGET_H
