#include "YUVOpenGLWidget.h"
#include <QOpenGLTexture>
#include <QOpenGLBuffer>

YUVOpenGLWidget::YUVOpenGLWidget(QWidget* parent) : QOpenGLWidget(parent)
{
    textureUniformY = 0;
    textureUniformU = 0;
    textureUniformV = 0;
    id_y = 0;
    id_u = 0;
    id_v = 0;
    m_pVSHader = NULL;
    m_pFSHader = NULL;
    m_pShaderProgram = NULL;
    m_pTextureY = NULL;
    m_pTextureU = NULL;
    m_pTextureV = NULL;
}

YUVOpenGLWidget::~YUVOpenGLWidget()
{
    releaseMemory();
}

void YUVOpenGLWidget::updateFrame(MediaFrameSharedPointer frame)
{
#if 1
    int src_width = frame->fmt.sub_fmt.video_fmt.width;
    int src_height = frame->fmt.sub_fmt.video_fmt.height;
    int stridey = frame->stride[0];
    int strideu = frame->stride[1];
    int stridev = frame->stride[2];
    int size = stridey * src_height + (strideu + stridev) * ((src_height + 1) / 2);

    m_videoFrame.reset(new krtc::MediaFrame(size));
    m_videoFrame->fmt.sub_fmt.video_fmt.type = krtc::SubMediaType::kSubTypeI420;
    m_videoFrame->fmt.sub_fmt.video_fmt.width = src_width;
    m_videoFrame->fmt.sub_fmt.video_fmt.height = src_height;
    m_videoFrame->stride[0] = stridey;
    m_videoFrame->stride[1] = strideu;
    m_videoFrame->stride[2] = stridev;
    m_videoFrame->data_len[0] = stridey * src_height;
    m_videoFrame->data_len[1] = strideu * ((src_height + 1) / 2);
    m_videoFrame->data_len[2] = stridev * ((src_height + 1) / 2);
    m_videoFrame->data[0] = new char[m_videoFrame->data_len[0]];
    m_videoFrame->data[1] = new char[m_videoFrame->data_len[1]];
    m_videoFrame->data[2] = new char[m_videoFrame->data_len[2]];
    memcpy(m_videoFrame->data[0], frame->data[0], frame->data_len[0]);
    memcpy(m_videoFrame->data[1], frame->data[1], frame->data_len[1]);
    memcpy(m_videoFrame->data[2], frame->data[2], frame->data_len[2]);

    update();

#else
    static FILE* fp = fopen("f://save_preview.yuv", "wb+");
    if (fp != NULL) {
        int width = frame->fmt.sub_fmt.video_fmt.width;
        int height = frame->fmt.sub_fmt.video_fmt.height;

        fwrite(frame->data[0], 1, width * height, fp);
        fwrite(frame->data[1], 1, width * height / 4, fp);
        fwrite(frame->data[2], 1, width * height / 4, fp);
        fflush(fp);
    }
#endif

}

void YUVOpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);

    // opengl��Ⱦ����������ɫ���������������
    // ��ɫ��������ʹ��openGL��ɫ����(OpenGL Shading Language,GLSL)��д��һ��С����,GLSL�ǹ�������OpenGL��ɫ��������
    // ��ʼ��������ɫ��������
    m_pVSHader = new QOpenGLShader(QOpenGLShader::Vertex, this);

    // ������ɫ��Դ��
    const char* vsrc = "attribute vec4 vertexIn; \
    attribute vec2 textureIn; \
    varying vec2 textureOut;  \
    void main(void)           \
    {                         \
        gl_Position = vertexIn; \
        textureOut = textureIn; \
    }";

    // ���붥����ɫ������
    m_pVSHader->compileSourceCode(vsrc);

    // ��ʼ��Ƭ����ɫ������gpu��yuvת����rgb
    m_pFSHader = new QOpenGLShader(QOpenGLShader::Fragment, this);

    // Ƭ����ɫ��Դ��
    const char* fsrc = "varying vec2 textureOut; \
    uniform sampler2D tex_y; \
    uniform sampler2D tex_u; \
    uniform sampler2D tex_v; \
    void main(void) \
    { \
        vec3 yuv; \
        vec3 rgb; \
        yuv.x = texture2D(tex_y, textureOut).r; \
        yuv.y = texture2D(tex_u, textureOut).r - 0.5; \
        yuv.z = texture2D(tex_v, textureOut).r - 0.5; \
        rgb = mat3( 1,       1,         1, \
                    0,       -0.39465,  2.03211, \
                    1.13983, -0.58060,  0) * yuv; \
        gl_FragColor = vec4(rgb, 1); \
    }";

    // ��glslԴ�����������������ɫ������
    m_pFSHader->compileSourceCode(fsrc);

    // ������ɫ����������
    m_pShaderProgram = new QOpenGLShaderProgram;
    // ��Ƭ����ɫ����ӵ���������
    m_pShaderProgram->addShader(m_pFSHader);
    // ��������ɫ����ӵ���������
    m_pShaderProgram->addShader(m_pVSHader);
    // ������vertexIn��ָ��λ��ATTRIB_VERTEX,�������ڶ�����ɫԴ������������
    m_pShaderProgram->bindAttributeLocation("vertexIn", ATTRIB_VERTEX);
    // ������textureIn��ָ��λ��ATTRIB_TEXTURE,�������ڶ�����ɫԴ������������
    m_pShaderProgram->bindAttributeLocation("textureIn", ATTRIB_TEXTURE);
    // ���������������뵽����ɫ������
    m_pShaderProgram->link();
    // ������������
    m_pShaderProgram->bind();

    // ��ȡ��ɫ���е����ݱ���tex_y, tex_u, tex_v��λ��,��Щ����������������
    // Ƭ����ɫ��Դ���п��Կ���
    textureUniformY = m_pShaderProgram->uniformLocation("tex_y");
    textureUniformU = m_pShaderProgram->uniformLocation("tex_u");
    textureUniformV = m_pShaderProgram->uniformLocation("tex_v");

    // �������
    static const GLfloat vertexVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         -1.0f, 1.0f,
         1.0f, 1.0f,
    };

    // �������
    static const GLfloat textureVertices[] = {
        0.0f,  1.0f,
        1.0f,  1.0f,
        0.0f,  0.0f,
        1.0f,  0.0f,
    };

    // ��������ATTRIB_VERTEX�Ķ������ֵ�Լ���ʽ
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, vertexVertices);
    // ��������ATTRIB_TEXTURE���������ֵ�Լ���ʽ
    glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices);
    // ����ATTRIB_VERTEX���Ե�����,Ĭ���ǹرյ�
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    // ����ATTRIB_TEXTURE���Ե�����,Ĭ���ǹرյ�
    glEnableVertexAttribArray(ATTRIB_TEXTURE);

    // �ֱ𴴽�y,u,v�������
    m_pTextureY = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_pTextureU = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_pTextureV = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_pTextureY->create();
    m_pTextureU->create();
    m_pTextureV->create();

    // ��ȡ����y��������������ֵ
    id_y = m_pTextureY->textureId();
    // ��ȡ����u��������������ֵ
    id_u = m_pTextureU->textureId();
    // ��ȡ����v��������������ֵ
    id_v = m_pTextureV->textureId();

    // ���ñ���ɫ
    glClearColor(0.3, 0.3, 0.3, 0.0);

}
void YUVOpenGLWidget::resizeGL(int w, int h)
{
    if (h == 0) // ��ֹ�����
    {
        h = 1;  // ������Ϊ1
    }

    if (w == 0) {
        w = 1;
    }

    glViewport(0, 0, w, h);
}

void YUVOpenGLWidget::paintGL()
{
    if ( nullptr == m_videoFrame) {
        return;
    }

    int width = m_videoFrame->fmt.sub_fmt.video_fmt.width;
    int height = m_videoFrame->fmt.sub_fmt.video_fmt.height;

    // ����y��������
    // ��������ԪGL_TEXTURE0
    glActiveTexture(GL_TEXTURE0);
    // ʹ������y������������
    glBindTexture(GL_TEXTURE_2D, id_y);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED,
        GL_UNSIGNED_BYTE, m_videoFrame->data[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // ����u��������
    // ��������ԪGL_TEXTURE1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, id_u);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width / 2, height / 2, 0, GL_RED,
        GL_UNSIGNED_BYTE, m_videoFrame->data[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // ����v��������
    // ��������ԪGL_TEXTURE2
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, id_v);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width / 2, height / 2, 0, GL_RED,
        GL_UNSIGNED_BYTE, m_videoFrame->data[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // ָ��y����Ҫʹ����ֵ ֻ����0,1,2�ȱ�ʾ����Ԫ������������opengl�����Ի��ĵط�
    // 0��Ӧ����ԪGL_TEXTURE0 1��Ӧ����ԪGL_TEXTURE1 2��Ӧ����ĵ�Ԫ
    glUniform1i(textureUniformY, 0);
    // ָ��u����Ҫʹ����ֵ
    glUniform1i(textureUniformU, 1);
    // ָ��v����Ҫʹ����ֵ
    glUniform1i(textureUniformV, 2);
    // ʹ�ö������鷽ʽ����ͼ��
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

}

void YUVOpenGLWidget::releaseMemory()
{
    if (m_pTextureY) {
        m_pTextureY->destroy();
        m_pTextureY = nullptr;
    }

    if (m_pTextureU) {
        m_pTextureU->destroy();
        m_pTextureU = nullptr;
    }

    if (m_pTextureV) {
        m_pTextureV->destroy();
        m_pTextureV = nullptr;
    }

    if (m_pShaderProgram) {
        m_pShaderProgram->removeAllShaders();
        m_pShaderProgram->release();
        m_pShaderProgram = nullptr;
    }
}

