///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ================
// This file contains the implementation of the `SceneManager` class, which is 
// responsible for managing the preparation and rendering of 3D scenes. It 
// handles textures, materials, lighting configurations, and object rendering.
//
// AUTHOR: Brian Battersby
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
//
// INITIAL VERSION: November 1, 2023
// LAST REVISED: December 1, 2024
//
// RESPONSIBILITIES:
// - Load, bind, and manage textures in OpenGL.
// - Define materials and lighting properties for 3D objects.
// - Manage transformations and shader configurations.
// - Render complex 3D scenes using basic meshes.
//
// NOTE: This implementation leverages external libraries like `stb_image` for 
// texture loading and GLM for matrix and vector operations.
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"
#include "ViewManager.h"   // added!: to check orthographic state
#include <chrono>  // added!: for per-frame timing
#include <cmath>   // added!: for sinf
#include <vector>  // added!: for std::vector

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
    const char* g_ModelName = "model";
    const char* g_ColorValueName = "objectColor";
    const char* g_TextureValueName = "objectTexture";
    const char* g_UseTextureName = "bUseTexture";
    const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
    m_pShaderManager = pShaderManager;
    m_basicMeshes = new ShapeMeshes();

    // added!: initialize texture table
    for (int i = 0; i < 16; ++i)
    {
        m_textureIDs[i].ID = -1;
        m_textureIDs[i].tag = "/0";
    }
    m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
    m_pShaderManager = NULL;
    delete m_basicMeshes;
    m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
    int width = 0;
    int height = 0;
    int colorChannels = 0;
    GLuint textureID = 0;

    // indicate to always flip images vertically when loaded
    stbi_set_flip_vertically_on_load(true);

    // try to parse the image data from the specified image file
    unsigned char* image = stbi_load(
        filename,
        &width,
        &height,
        &colorChannels,
        0);

    // if the image was successfully read from the image file
    if (image)
    {
        std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // added
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // added

        // if the loaded image is in RGB format
        if (colorChannels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        // if the loaded image is in RGBA format - it supports transparency
        else if (colorChannels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
            return false;
        }

        // generate the texture mipmaps for mapping textures to lower resolutions
        glGenerateMipmap(GL_TEXTURE_2D);

        // free the image data from local memory
        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        // register the loaded texture and associate it with the special tag string
        m_textureIDs[m_loadedTextures].ID = textureID;
        m_textureIDs[m_loadedTextures].tag = tag;
        m_loadedTextures++;

        return true;
    }

    std::cout << "Could not load image:" << filename << std::endl;

    // Error loading the image
    return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
    for (int i = 0; i < m_loadedTextures; i++)
    {
        // bind textures on corresponding texture units
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
    }
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
    for (int i = 0; i < m_loadedTextures; i++)
    {
        glGenTextures(1, &m_textureIDs[i].ID);
    }
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
    int textureID = -1;
    int index = 0;
    bool bFound = false;

    while ((index < m_loadedTextures) && (bFound == false))
    {
        if (m_textureIDs[index].tag.compare(tag) == 0)
        {
            textureID = m_textureIDs[index].ID;
            bFound = true;
        }
        else
            index++;
    }

    return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
    int textureSlot = -1;
    int index = 0;
    bool bFound = false;

    while ((index < m_loadedTextures) && (bFound == false))
    {
        if (m_textureIDs[index].tag.compare(tag) == 0)
        {
            textureSlot = index;
            bFound = true;
        }
        else
            index++;
    }

    return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
    if (m_objectMaterials.size() == 0)
    {
        return(false);
    }

    int index = 0;
    bool bFound = false;
    while ((index < (int)m_objectMaterials.size()) && (bFound == false))
    {
        if (m_objectMaterials[index].tag.compare(tag) == 0)
        {
            bFound = true;
            material.diffuseColor = m_objectMaterials[index].diffuseColor;
            material.specularColor = m_objectMaterials[index].specularColor;
            material.shininess = m_objectMaterials[index].shininess;
        }
        else
        {
            index++;
        }
    }

    return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
    glm::vec3 scaleXYZ,
    float XrotationDegrees,
    float YrotationDegrees,
    float ZrotationDegrees,
    glm::vec3 positionXYZ)
{
    // variables for this method
    glm::mat4 modelView;
    glm::mat4 scale;
    glm::mat4 rotationX;
    glm::mat4 rotationY;
    glm::mat4 rotationZ;
    glm::mat4 translation;

    // set the scale value in the transform buffer
    scale = glm::scale(scaleXYZ);
    // set the rotation values in the transform buffer
    rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
    rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
    rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
    // set the translation value in the transform buffer
    translation = glm::translate(positionXYZ);

    modelView = translation * rotationZ * rotationY * rotationX * scale;

    if (NULL != m_pShaderManager)
    {
        m_pShaderManager->setMat4Value(g_ModelName, modelView);
    }
}

/***********************************************************
 *  SetTransformationsMatrix() - Matrix version
 *
 *  This method is used for setting the transform buffer
 *  using a pre-calculated model matrix.
 ***********************************************************/
void SceneManager::SetTransformationsMatrix(const glm::mat4& modelMatrix)
{
    if (NULL != m_pShaderManager)
    {
        m_pShaderManager->setMat4Value(g_ModelName, modelMatrix);
    }
}

/***********************************************************
 *  added! DrawRopeBetweenPoints() - Utility function
 *
 *  This method draws a cylinder rope between two points
 *  using proper matrix transformation.
 ***********************************************************/
void SceneManager::DrawRopeBetweenPoints(const glm::vec3& pointA, const glm::vec3& pointB, float radius, float lengthMultiplier)
{
    glm::vec3 dir = pointB - pointA;
    float baseLength = glm::length(dir);
    float actualLength = baseLength * lengthMultiplier;  // Apply multiplier

    if (actualLength < 0.001f) return; // Prevent division by zero or weird visuals

    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 normalizedDir = glm::normalize(dir);
    glm::vec3 axis = glm::cross(up, normalizedDir);
    float angle = glm::acos(glm::clamp(glm::dot(up, normalizedDir), -1.0f, 1.0f));

    // Start building the model matrix
    glm::mat4 model = glm::mat4(1.0f);

    // 1. Translate to pointA (tent top) - START HERE
    model = glm::translate(model, pointA);

    // 2. Rotate to align Y-axis with direction vector
    if (glm::length(axis) > 0.0001f)
        model = glm::rotate(model, angle, glm::normalize(axis));

    // 3. Scale to match desired length and radius
    // Use actualLength (not actualLength * 0.5f) so it extends the full distance
    model = glm::scale(model, glm::vec3(radius, actualLength, radius));

    // 4. Apply the matrix
    SetTransformationsMatrix(model);

    // Material + visual setup
    SetShaderColor(0.5f, 0.3f, 0.1f, 1.0f);
    SetShaderTexture("canvas2");
    SetTextureUVScale(0.1f, 20.0f);

    m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.4f, 0.25f, 0.1f));
    m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.0f));
    m_pShaderManager->setFloatValue("material.shininess", 1.0f);

    // Finally draw it
    m_basicMeshes->DrawCylinderMesh();
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
    float redColorValue,
    float greenColorValue,
    float blueColorValue,
    float alphaValue)
{
    // variables for this method
    glm::vec4 currentColor;

    currentColor.r = redColorValue;
    currentColor.g = greenColorValue;
    currentColor.b = blueColorValue;
    currentColor.a = alphaValue;

    if (NULL != m_pShaderManager)
    {
        m_pShaderManager->setIntValue(g_UseTextureName, false);
        m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
    }
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
    std::string textureTag)
{
    if (NULL != m_pShaderManager)
    {
        m_pShaderManager->setIntValue(g_UseTextureName, true);

        int textureID = -1;
        textureID = FindTextureSlot(textureTag);
        m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
    }
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
    if (NULL != m_pShaderManager)
    {
        m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
    }
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
    std::string materialTag)
{
    if (m_objectMaterials.size() > 0)
    {
        OBJECT_MATERIAL material;
        bool bReturn = false;

        bReturn = FindMaterial(materialTag, material);
        if (bReturn == true)
        {
            m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
            m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
            m_pShaderManager->setFloatValue("material.shininess", material.shininess);
        }
    }
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
    // only one instance of a particular mesh needs to be
    // loaded in memory no matter how many times it is drawn
    // in the rendered 3D scene

    // main textures for this scene
    m_loadedTextures = 0;
    for (int i = 0; i < 16; ++i) { m_textureIDs[i].ID = -1; m_textureIDs[i].tag = "/0"; }

    // relative to executable working dir; keeping files in ./textures/
    CreateGLTexture("./textures/grass.jpg", "grass");
    CreateGLTexture("./textures/tree-bark.jpg", "bark");
    CreateGLTexture("./textures/granite.jpeg", "granite");
    CreateGLTexture("./textures/moon.jpg", "moon");
    CreateGLTexture("./textures/canvas.jpg", "canvas");
    CreateGLTexture("./textures/canvas2.jpg", "canvas2");
    CreateGLTexture("./textures/pebblestone.jpg", "pebblestone");
    CreateGLTexture("./textures/background.png", "background");
    CreateGLTexture("./textures/rope.png", "rope");
    CreateGLTexture("./textures/pine-needle.jpg", "pine-needle");
    CreateGLTexture("./textures/tan-leather.jpg", "tan-leather");

    BindGLTextures();

    // --------------------------------------------------------------------
    // campfire-centric lighting (enable shader lighting + define lights)
    // --------------------------------------------------------------------
    m_pShaderManager->setBoolValue(g_UseLightingName, true);

    // defensively deactivate all point lights first
    for (int i = 0; i < 5; ++i) {
        std::string b = "pointLights[" + std::to_string(i) + "].";
        m_pShaderManager->setBoolValue(b + "bActive", false);
    }

    // primary light = CAMPFIRE (warm, extended range to reach backpack)
    {
        const std::string b = "pointLights[0].";
        const glm::vec3 campfirePos = glm::vec3(0.0f, 0.8f, 0.0f);
        m_pShaderManager->setVec3Value(b + "position", campfirePos);
        m_pShaderManager->setVec3Value(b + "ambient", glm::vec3(0.10f, 0.07f, 0.04f));
        m_pShaderManager->setVec3Value(b + "diffuse", glm::vec3(1.00f, 0.70f, 0.30f));
        m_pShaderManager->setVec3Value(b + "specular", glm::vec3(0.90f, 0.60f, 0.30f));
        m_pShaderManager->setFloatValue(b + "constant", 1.0f);
        m_pShaderManager->setFloatValue(b + "linear", 0.07f);  // Reduced for longer range
        m_pShaderManager->setFloatValue(b + "quadratic", 0.017f); // Reduced for longer range
        m_pShaderManager->setBoolValue(b + "bActive", true);
    }

    // faint moonlight as directional fill (defaults; adjusted per-frame in RenderScene)
    m_pShaderManager->setVec3Value("directionalLight.direction", glm::vec3(-0.25f, -1.0f, -0.35f));
    m_pShaderManager->setVec3Value("directionalLight.ambient", glm::vec3(0.06f));
    m_pShaderManager->setVec3Value("directionalLight.diffuse", glm::vec3(0.12f));
    m_pShaderManager->setVec3Value("directionalLight.specular", glm::vec3(0.10f));
    m_pShaderManager->setBoolValue("directionalLight.bActive", true);

    // keep spotlight path off unless you add one later
    m_pShaderManager->setBoolValue("spotLight.bActive", false);

    // --------------------------------------------------------------------

    m_basicMeshes->LoadPlaneMesh();
    m_basicMeshes->LoadCylinderMesh();
    m_basicMeshes->LoadSphereMesh();
    m_basicMeshes->LoadBoxMesh();
    m_basicMeshes->LoadConeMesh();
    m_basicMeshes->LoadPrismMesh();
}


/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
    // declare the variables for the transformations
    glm::vec3 scaleXYZ;
    float XrotationDegrees = 0.0f;
    float YrotationDegrees = 0.0f;
    float ZrotationDegrees = 0.0f;
    glm::vec3 positionXYZ;

    // per-frame time for animation (portable; uses <chrono>)
    static const auto t0 = std::chrono::steady_clock::now();
    const auto now = std::chrono::steady_clock::now();
    const float timeSeconds = std::chrono::duration<float>(now - t0).count();

    // --------------------------------------------------------------------
    // per-frame CAMPFIRE LIGHT FLICKER (updates pointLights[0])
    // --------------------------------------------------------------------
    {
        const glm::vec3 basePos = glm::vec3(0.0f, 0.8f, 0.0f);
        const glm::vec3 baseAmb = glm::vec3(0.10f, 0.07f, 0.04f);
        const glm::vec3 baseDif = glm::vec3(1.00f, 0.70f, 0.30f);
        const glm::vec3 baseSpec = glm::vec3(0.90f, 0.60f, 0.30f);

        float f1 = 0.5f + 0.5f * sinf(timeSeconds * 6.2f + 1.3f);
        float f2 = 0.5f + 0.5f * sinf(timeSeconds * 3.9f + 2.1f);
        float f3 = 0.5f + 0.5f * sinf(timeSeconds * 9.1f + 0.5f);
        float flicker = 0.80f + 0.40f * (0.55f * f1 + 0.30f * f2 + 0.15f * f3); // ~0.8..1.2

        glm::vec3 jitter(
            0.03f * sinf(timeSeconds * 4.7f),
            0.02f * sinf(timeSeconds * 5.3f + 1.7f),
            0.03f * cosf(timeSeconds * 4.1f)
        );

        const std::string b = "pointLights[0].";
        m_pShaderManager->setVec3Value(b + "position", basePos + jitter);
        m_pShaderManager->setVec3Value(b + "ambient", baseAmb * (0.85f + 0.15f * flicker));
        m_pShaderManager->setVec3Value(b + "diffuse", baseDif * flicker);
        m_pShaderManager->setVec3Value(b + "specular", baseSpec * (0.90f + 0.10f * flicker));
        
        // Update falloff for extended range
        m_pShaderManager->setFloatValue(b + "linear", 0.07f);  // Extended range
        m_pShaderManager->setFloatValue(b + "quadratic", 0.017f); // Extended range
    }
    // --------------------------------------------------------------------

    // set the XYZ scale for the mesh
    scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

    // set the XYZ rotation for the mesh
    XrotationDegrees = 0.0f;
    YrotationDegrees = 0.0f;
    ZrotationDegrees = 0.0f;

    // set the XYZ position for the mesh
    positionXYZ = glm::vec3(0.0f, 0.0f, -2.0f);  // Brought forward a tiny bit

    // set the transformations into memory to be used on the drawn meshes
    SetTransformations(
        scaleXYZ,
        XrotationDegrees,
        YrotationDegrees,
        ZrotationDegrees,
        positionXYZ);

    // Ground plane (hidden in orthographic view)
    if (!ViewManager::IsOrthographic())
    {
        SetShaderColor(1, 1, 1, 1);

        SetTextureUVScale(12.0f, 12.0f);
        SetShaderTexture("grass");

        // mild specular so campfire highlights show on grass
        m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.20f));
        m_pShaderManager->setFloatValue("material.shininess", 16.0f);

        m_basicMeshes->DrawPlaneMesh();

        // Background wall - same as ground plane but standing up
        SetShaderColor(1, 1, 1, 1);  // Full white to show texture properly
        SetTextureUVScale(0.5f, 0.5f);  // Try smaller UV scale to see if texture appears
        SetShaderTexture("background");  // Use background texture
        
        // Material properties for lighting - brighter values
        m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(1.0f, 1.0f, 1.0f));  // Full white diffuse
        m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.1f));  // Lower specular
        m_pShaderManager->setFloatValue("material.shininess", 8.0f);  // Lower shininess
        
        // Same as ground plane but rotated 90 degrees to stand up
        SetTransformations(
            glm::vec3(20.0f, 1.0f, 10.0f),  // Exact same scale as ground plane
            90.0f, 0.0f, 0.0f,              // Rotate 90 degrees around X to make it vertical
            glm::vec3(0.0f, 10.0f, -12.0f)  // Brought forward a tiny bit to match ground
        );

        m_basicMeshes->DrawPlaneMesh();

        // optional wire overlay - DISABLED to fix floating lines issue
        /*
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-1.0f, -1.0f);
        glLineWidth(1.5f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
        m_basicMeshes->DrawPlaneMesh();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_POLYGON_OFFSET_LINE);
        */
    }

    /*************** CAMPFIRE OBJECT START ******************/

    /* ============================ helpers ============================ */

    // cylinder with optional thin wireframe overlay (logs)
    auto drawCyl = [&](const glm::vec3& s,
        float xDeg, float yDeg, float zDeg,
        const glm::vec3& p,
        const glm::vec4& rgba,
        bool withWire)
        {
            SetTransformations(s, xDeg, yDeg, zDeg, p);
            SetShaderColor(rgba.r, rgba.g, rgba.b, rgba.a);

            SetTextureUVScale(2.5f, 4.0f);
            SetShaderTexture("bark");

            // wood material
            m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.42f, 0.28f, 0.15f));
            m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.08f, 0.06f, 0.04f));
            m_pShaderManager->setFloatValue("material.shininess", 12.0f);

            m_basicMeshes->DrawCylinderMesh();

            if (withWire)
            {
                glEnable(GL_POLYGON_OFFSET_LINE);
                glPolygonOffset(-1.0f, -1.0f);
                glLineWidth(0.55f);
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                SetShaderColor(0.08f, 0.08f, 0.08f, 1.0f);
                m_basicMeshes->DrawCylinderMesh();
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glDisable(GL_POLYGON_OFFSET_LINE);
            }
        };

    // stones (spheres) with optional wire overlay
    auto drawStone = [&](const glm::vec3& s,
        const glm::vec3& p,
        const glm::vec4& rgba,
        bool withWire,
        const char* texTag)
        {
            SetTransformations(s, 0.0f, 0.0f, 0.0f, p);
            SetShaderColor(rgba.r, rgba.g, rgba.b, rgba.a);

            SetTextureUVScale(1.4f, 1.4f);
            SetShaderTexture(texTag);

            // stone material
            m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.55f, 0.55f, 0.56f));
            m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.18f, 0.18f, 0.20f));
            m_pShaderManager->setFloatValue("material.shininess", 24.0f);

            m_basicMeshes->DrawSphereMesh();

            if (withWire)
            {
                glEnable(GL_POLYGON_OFFSET_LINE);
                glPolygonOffset(-1.0f, -1.0f);
                glLineWidth(0.5f);
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                SetShaderColor(0.05f, 0.05f, 0.05f, 1.0f);
                m_basicMeshes->DrawSphereMesh();
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glDisable(GL_POLYGON_OFFSET_LINE);
            }
        };

    // embers (additive)
    auto drawEmber = [&](const glm::vec3& s, const glm::vec3& p)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);

            m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.06f));
            m_pShaderManager->setFloatValue("material.shininess", 8.0f);

            // hot core
            SetTransformations(s, 0.0f, 0.0f, 0.0f, p);
            SetShaderColor(1.00f, 0.82f, 0.35f, 0.90f);
            m_basicMeshes->DrawSphereMesh();

            // mid glow
            glm::vec3 sMid = s * glm::vec3(1.35f, 1.00f, 1.35f);
            SetTransformations(sMid, 0.0f, 0.0f, 0.0f, p);
            SetShaderColor(1.00f, 0.55f, 0.15f, 0.55f);
            m_basicMeshes->DrawSphereMesh();

            // cool halo
            glm::vec3 sCool = s * glm::vec3(1.85f, 1.00f, 1.85f);
            SetTransformations(sCool, 0.0f, 0.0f, 0.0f, p);
            SetShaderColor(0.92f, 0.20f, 0.05f, 0.35f);
            m_basicMeshes->DrawSphereMesh();

            glDisable(GL_BLEND);
        };

    // REALISTIC FLAME SYSTEM - Multiple organic flame shapes with complex animation
    auto drawRealisticFlame = [&](const glm::vec3& p, float h, float r, float leanX, float yaw, float roll,
        const glm::vec4& cInner, const glm::vec4& cMid, const glm::vec4& cOuter)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(GL_TRUE);  // Enable depth testing for proper rendering order
            glDepthFunc(GL_LESS);  // Ensure proper depth comparison
            GLboolean cullWasOn = glIsEnabled(GL_CULL_FACE);
            if (cullWasOn) glDisable(GL_CULL_FACE);

            // Enhanced flame uniforms for more realistic effects
            m_pShaderManager->setBoolValue("bEmissive", true);
            m_pShaderManager->setBoolValue("bFlame", true);
            m_pShaderManager->setFloatValue("flameTime", timeSeconds);
            m_pShaderManager->setFloatValue("flameBaseY", p.y);
            m_pShaderManager->setFloatValue("flameHeight", h);
            m_pShaderManager->setFloatValue("flameWobbleAmp", 0.15f);  // Increased wobble
            m_pShaderManager->setFloatValue("flameTwist", 1.2f);     // More twist

            // Complex flicker and turbulence for realism
            float seed = p.x * 3.17f + p.z * 5.41f;
            float f1 = 0.5f + 0.5f * sinf(timeSeconds * 8.5f + seed);
            float f2 = 0.5f + 0.5f * sinf(timeSeconds * 5.2f + 1.7f * seed);
            float f3 = 0.5f + 0.5f * sinf(timeSeconds * 12.1f + 2.3f * seed);
            float flicker = 0.80f + 0.40f * (0.4f * f1 + 0.4f * f2 + 0.2f * f3);
            
            // Multi-frequency wave patterns for organic movement
            float waveX = 3.5f * sinf(timeSeconds * 2.1f + seed * 0.8f) + 
                         1.2f * sinf(timeSeconds * 7.3f + seed * 1.4f);
            float waveY = 3.5f * sinf(timeSeconds * 1.6f + seed * 1.2f) + 
                         1.2f * sinf(timeSeconds * 6.8f + seed * 0.9f);
            float waveZ = 2.0f * sinf(timeSeconds * 3.2f + seed * 1.1f);
            
            float hA = h * (0.85f + 0.30f * flicker);
            float rA = r * (0.90f + 0.20f * flicker);

            // FLAME CORE - Bright white-hot center
            {
                glm::vec3 s1(rA * 0.4f, hA * 0.7f, rA * 0.4f);
                SetTransformations(s1, leanX + waveX * 0.3f, yaw + waveY * 0.3f, roll + waveZ * 0.2f, 
                    p + glm::vec3(0, s1.y * 0.15f, 0));
                SetShaderColor(cInner.r, cInner.g, cInner.b, cInner.a * 1.2f);
                m_basicMeshes->DrawConeMesh();
            }
            
            // FLAME INNER LAYER - Bright orange core
            {
                glm::vec3 s2(rA * 0.65f, hA * 0.8f, rA * 0.65f);
                SetTransformations(s2, leanX + waveX * 0.6f, yaw + waveY * 0.6f, roll + waveZ * 0.4f, 
                    p + glm::vec3(0, s2.y * 0.08f, 0));
                SetShaderColor(cMid.r, cMid.g, cMid.b, cMid.a);
                m_basicMeshes->DrawConeMesh();
            }
            
            // FLAME MIDDLE LAYER - Orange to red transition
            {
                glm::vec3 s3(rA * 0.9f, hA * 0.9f, rA * 0.9f);
                SetTransformations(s3, leanX + waveX * 0.8f, yaw + waveY * 0.8f, roll + waveZ * 0.6f, 
                    p + glm::vec3(0, s3.y * 0.04f, 0));
                SetShaderColor(cOuter.r, cOuter.g, cOuter.b, cOuter.a * 0.8f);
                m_basicMeshes->DrawConeMesh();
            }
            
            // FLAME OUTER HALO - Red tips with turbulence
            {
                glm::vec3 s4(rA * 1.2f, hA * 1.1f, rA * 1.2f);
                SetTransformations(s4, leanX + waveX, yaw + waveY, roll + waveZ, p);
                SetShaderColor(cOuter.r * 0.8f, cOuter.g * 0.3f, cOuter.b * 0.1f, cOuter.a * 0.6f);
                m_basicMeshes->DrawConeMesh();
            }
            
            // FLAME TIPS - Additional smaller flames for realism
            for (int tip = 0; tip < 3; ++tip) {
                float tipAngle = (tip * 120.0f) + timeSeconds * 45.0f;
                float tipRad = glm::radians(tipAngle);
                float tipX = 0.3f * rA * cos(tipRad);
                float tipZ = 0.3f * rA * sin(tipRad);
                float tipH = hA * (0.4f + 0.2f * f1);
                
                glm::vec3 s5(rA * 0.3f, tipH, rA * 0.3f);
                SetTransformations(s5, leanX + waveX * 1.2f, yaw + waveY * 1.2f, roll + waveZ * 0.8f, 
                    p + glm::vec3(tipX, tipH * 0.5f, tipZ));
                SetShaderColor(cOuter.r * 0.9f, cOuter.g * 0.4f, cOuter.b * 0.1f, cOuter.a * 0.4f);
                m_basicMeshes->DrawConeMesh();
            }

            // restore state
            m_pShaderManager->setBoolValue("bFlame", false);
            m_pShaderManager->setBoolValue("bEmissive", false);
            if (cullWasOn) glEnable(GL_CULL_FACE);
            glDisable(GL_BLEND);
        };

    // utilities
    auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
    auto jitter = [](int i, float amp) { return ((i % 2 == 0) ? +amp : -amp); };
    auto makeRockScale = [&](float r) { return glm::vec3(r, r * 0.6f, r); };
    auto makeCoalScale = [&](float r) { return glm::vec3(r, r * 0.45f, r); };

    /* ============================== colors ============================== */
    const glm::vec4 woodColor = glm::vec4(0.50f, 0.34f, 0.20f, 1.0f);
    const glm::vec4 stoneColor = glm::vec4(0.45f, 0.45f, 0.45f, 1.0f);

    // REALISTIC FLAME PALETTE - More natural fire colors
    const glm::vec4 C_INNER = glm::vec4(1.00f, 1.00f, 0.95f, 0.98f);  // White-hot core
    const glm::vec4 C_MID = glm::vec4(1.00f, 0.75f, 0.35f, 0.75f);    // Bright orange
    const glm::vec4 C_OUTER = glm::vec4(0.95f, 0.35f, 0.05f, 0.45f); // Deep red

    /* ================================ logs =============================== */
    {
        const float groundY_logs = 0.0f;
        const int   logCount = 8;
        const float logRadius = 0.26f;
        const float logLength = 3.20f;
        const float tiltUpDegrees = 18.0f;
        const float epsilonLift = 0.20f;
        const glm::vec3 S_LOG(logRadius, logLength, logRadius);
        const float ringR = logLength * 0.60f;

        auto placeRadialLog = [&](float deg, float yawJit, float rollJit, float liftJit)
            {
                float aRad = glm::radians(deg);
                glm::vec3 dir(cos(aRad), 0.0f, sin(aRad));

                float xDeg = 90.0f + tiltUpDegrees;
                float yDeg = deg + 180.0f + yawJit;
                float zDeg = rollJit;

                float halfRise = sin(glm::radians(tiltUpDegrees)) * (logLength * 0.5f);

                glm::vec3 center = (ringR - (logLength * 0.5f)) * dir * -1.0f;
                center.y = groundY_logs + logRadius + halfRise + liftJit + epsilonLift;

                drawCyl(S_LOG, xDeg, yDeg, zDeg, center, woodColor, false);
            };

        for (int i = 0; i < logCount; ++i)
        {
            float a = (360.0f / logCount) * i;
            float yawJ = (i % 2 == 0 ? 2.0f : -2.0f);
            float rollJ = (i % 3 == 0 ? 1.5f : (i % 3 == 1 ? -1.0f : 0.5f));
            float yJ = (i % 4 == 0 ? 0.03f : (i % 4 == 1 ? 0.00f : (i % 4 == 2 ? 0.02f : 0.01f)));
            placeRadialLog(a, yawJ, rollJ, yJ);
        }
    }

    /* =============================== stones ============================== */
    {
        const float stoneLift = 0.03f;

        // inner scattered stones
        {
            const int   innerCount = 12;
            const float innerR_min = 0.35f;
            const float innerR_max = 1.10f;
            const float baseRadius = 0.25f;
            const float innerSizeMin = baseRadius * 0.55f;
            const float innerSizeMax = baseRadius * 0.90f;

            for (int i = 0; i < innerCount; ++i)
            {
                float aDeg = (360.0f / innerCount) * i
                    + (i % 3 == 0 ? 12.0f : (i % 3 == 1 ? -7.0f : 3.0f));
                float aRad = glm::radians(aDeg);

                float rNorm = (i % 5) / 4.0f;
                float r = innerR_min + (innerR_max - innerR_min) * rNorm
                    + ((i % 2 == 0) ? 0.06f : -0.04f);

                float x = r * cos(aRad);
                float z = r * sin(aRad);

                float sNorm = ((i * 37) % 100) / 100.0f;
                float rSize = innerSizeMin + (innerSizeMax - innerSizeMin) * sNorm;

                float y = rSize * 0.30f + stoneLift;

                drawStone(makeRockScale(rSize), glm::vec3(x, y, z), stoneColor, false, "pebblestone");
            }
        }

        // mid ring stones
        {
            const int   ringStoneCount = 16;
            const float ringRadius = 2.00f;
            const float baseRadius = 0.25f;

            for (int i = 0; i < ringStoneCount; ++i)
            {
                float a = glm::radians((360.0f / ringStoneCount) * i);
                float rJit = ((i % 3) - 1) * 0.08f;

                float x = (ringRadius + rJit) * cos(a);
                float z = (ringRadius + rJit) * sin(a);

                float rSize = baseRadius * (0.85f + 0.25f * ((i % 4) / 3.0f));
                float y = rSize * 0.30f + stoneLift;

                drawStone(makeRockScale(rSize), glm::vec3(x, y, z), stoneColor, false, "pebblestone");
            }
        }

        // circular ground patch inside the guard ring
        {
            const float patchRadius = 2.95f;
            const float patchHeight = 0.01f;
            const float yLift = 0.0015f;

            SetTransformations(
                glm::vec3(patchRadius, patchHeight, patchRadius),
                0.0f, 0.0f, 0.0f,
                glm::vec3(0.0f, yLift, 0.0f));

            SetShaderColor(1, 1, 1, 1);
            SetTextureUVScale(1.2f, 1.2f);
            SetShaderTexture("pebblestone");

            m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.20f));
            m_pShaderManager->setFloatValue("material.shininess", 24.0f);

            m_basicMeshes->DrawCylinderMesh();
        }

        // outer guard ring boulders (overlapping spheres)
        {
            const int   bigCount = 18;
            const float bigRingRadius = 3.10f;
            const float bigBase = 0.48f;
            const bool  withWireBlob = false;

            auto drawBlobPart = [&](const glm::vec3& s,
                const glm::vec3& p,
                const glm::vec4& rgba,
                bool withWire)
                {
                    SetTransformations(s, 0.0f, 0.0f, 0.0f, p);
                    SetShaderColor(rgba.r, rgba.g, rgba.b, rgba.a);

                    SetTextureUVScale(1.2f, 1.2f);
                    SetShaderTexture("granite");

                    m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.60f, 0.60f, 0.62f));
                    m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.25f, 0.25f, 0.27f));
                    m_pShaderManager->setFloatValue("material.shininess", 32.0f);

                    m_basicMeshes->DrawSphereMesh();

                    if (withWire)
                    {
                        glEnable(GL_POLYGON_OFFSET_LINE);
                        glPolygonOffset(-1.0f, -1.0f);
                        glLineWidth(0.5f);
                        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                        SetShaderColor(0.05f, 0.05f, 0.05f, 1.0f);
                        m_basicMeshes->DrawSphereMesh();
                        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                        glDisable(GL_POLYGON_OFFSET_LINE);
                    }
                };

            for (int i = 0; i < bigCount; ++i)
            {
                float a = glm::radians((360.0f / bigCount) * i);
                float rJit = ((i % 4) - 1.5f) * 0.18f;

                float x = (bigRingRadius + rJit) * cos(a);
                float z = (bigRingRadius + rJit) * sin(a);

                float sizeJ = (i % 5 == 0 ? 1.30f :
                    (i % 5 == 1 ? 1.15f :
                        (i % 5 == 2 ? 0.95f :
                            (i % 5 == 3 ? 1.05f : 0.85f))));
                float thisBase = bigBase * sizeJ;

                float inward = ((i % 2) ? -0.07f : 0.05f);
                x += inward * cos(a);
                z += inward * sin(a);

                float tint = 0.92f + 0.06f * float(i % 4);
                glm::vec4 col = glm::vec4(stoneColor.r * tint,
                    stoneColor.g * tint,
                    stoneColor.b * tint,
                    1.0f);

                glm::vec3 s0 = glm::vec3(
                    thisBase * (1.20f + 0.25f * ((i + 1) % 3)),
                    thisBase * (0.70f + 0.20f * ((i + 2) % 3)),
                    thisBase * (1.10f + 0.30f * ((i + 3) % 3))
                );
                glm::vec3 s1 = s0 * glm::vec3(0.70f, 0.80f, 0.65f);
                glm::vec3 s2 = s0 * glm::vec3(0.60f, 0.72f, 0.78f);
                glm::vec3 s3 = s0 * glm::vec3(0.45f, 0.55f, 0.50f);

                glm::vec3 off1 = glm::vec3(0.16f, 0.04f, -0.10f) * (((i % 2) == 0) ? 1.0f : -1.0f);
                glm::vec3 off2 = glm::vec3(-0.12f, 0.02f, 0.14f) * ((((i + 1) % 2) == 0) ? 1.0f : -1.0f);
                glm::vec3 off3 = glm::vec3(0.04f, 0.06f, 0.03f);

                float yHalfMax = glm::max(glm::max(s0.y, s1.y), glm::max(s2.y, s3.y));
                float baseY = 0.0f + 0.03f + yHalfMax * 0.30f;

                glm::vec3 pCenter(x, baseY, z);
                drawBlobPart(s0, pCenter, col, withWireBlob);
                drawBlobPart(s1, pCenter + off1, col, withWireBlob);
                drawBlobPart(s2, pCenter + off2, col, withWireBlob);
                drawBlobPart(s3, pCenter + off3, col, withWireBlob);
            }
        }
    }

    /* =============================== embers ============================== */
    {
        const float groundY_embers = 0.0f;
        const float emberLift = 0.03f;
        const float emberBase = 0.11f;

        const int   coreCount = 18;
        const int   rimCount = 22;

        const float coreR_min = 0.20f;
        const float coreR_max = 0.80f;
        const float rimR = 1.25f;

        // core embers
        for (int i = 0; i < coreCount; ++i)
        {
            float aDeg = (360.0f / coreCount) * i
                + (i % 3 == 0 ? 8.0f : (i % 3 == 1 ? -5.0f : 3.0f));
            float aRad = glm::radians(aDeg);

            float rNorm = (i % 7) / 6.0f;
            float r = coreR_min + (coreR_max - coreR_min) * rNorm
                + ((i % 2 == 0) ? 0.04f : -0.03f);

            float sNorm = ((i * 37) % 100) / 100.0f;
            float rSize = emberBase * (0.85f + 0.35f * sNorm);
            glm::vec3 s = makeCoalScale(rSize);

            float y = groundY_embers + emberLift + s.y * 0.30f;

            glm::vec3 p(r * cos(aRad), y, r * sin(aRad));
            drawEmber(s, p);
        }

        // rim embers
        for (int i = 0; i < rimCount; ++i)
        {
            float aDeg = (360.0f / rimCount) * i
                + (i % 4 == 0 ? -6.0f : (i % 4 == 1 ? 4.0f : (i % 4 == 2 ? -2.0f : 2.0f)));
            float aRad = glm::radians(aDeg);

            float rJit = ((i % 3) - 1) * 0.06f;
            float x = (rimR + rJit) * cos(aRad);
            float z = (rimR + rJit) * sin(aRad);

            float rSize = emberBase * (0.75f + 0.30f * ((i % 5) / 4.0f));
            glm::vec3 s = makeCoalScale(rSize);

            float y = groundY_embers + emberLift + s.y * 0.30f;
            drawEmber(s, glm::vec3(x, y, z));
        }
    }

    /* =============================== flames ============================== */
    {
        const float groundY_flame = 0.0f;
        const float baseLift = 0.02f;

        const int   radialCount = 12; // flames around inner log tips
        const float innerRingR = 0.90f;

        const int   centerCount = 5; // small central cluster

        // height and base radius ranges
        const float H_center_min = 0.75f, H_center_max = 3.15f;
        const float R_center_min = 0.48f, R_center_max = 0.60f;

        const float H_ring_min = 0.75f, H_ring_max = 3.10f;
        const float R_ring_min = 0.10f, R_ring_max = 0.50f;

        // central flames
        for (int i = 0; i < centerCount; ++i)
        {
            float tH = ((i * 37) % 100) / 100.0f;
            float tR = ((i * 53) % 100) / 100.0f;

            float h = lerp(H_center_min, H_center_max, tH);
            float r = lerp(R_center_min, R_center_max, tR);

            float aDeg = 360.0f * (i / float(centerCount));
            float aRad = glm::radians(aDeg);
            float rad = 0.18f + 0.10f * ((i % 3) / 2.0f);

            glm::vec3 p(rad * cos(aRad), groundY_flame + baseLift, rad * sin(aRad));

            float leanX = 2.0f * ((i % 2 == 0) ? 1.0f : -1.0f);
            float yaw = aDeg + 180.0f;
            float roll = jitter(i, 1.5f);

            drawRealisticFlame(p, h, r, leanX, yaw, roll, C_INNER, C_MID, C_OUTER);
        }

        // ring flames near inner log ends
        for (int i = 0; i < radialCount; ++i)
        {
            float aDeg = (360.0f / radialCount) * i;
            float aRad = glm::radians(aDeg);

            float rj = ((i % 3) - 1) * 0.06f;

            glm::vec3 p((innerRingR + rj) * cos(aRad),
                groundY_flame + baseLift,
                (innerRingR + rj) * sin(aRad));

            float h = lerp(H_ring_min, H_ring_max, ((i * 29) % 100) / 100.0f);
            float r = lerp(R_ring_min, R_ring_max, ((i * 47) % 100) / 100.0f);

            float leanX = 6.0f + jitter(i, 1.5f);
            float yaw = aDeg + 180.0f;
            float roll = jitter(i, 2.0f);

            drawRealisticFlame(p, h, r, leanX, yaw, roll, C_INNER, C_MID, C_OUTER);
        }
    }

    /************************ CAMPFIRE OBJECT END ************************/


 /******** added! TENT OBJECT START ********/
    {
        const glm::vec3 tentCenter = glm::vec3(-7.0f, 0.0f, -6.0f);
        const float tentWidth = 7.8f;
        const float tentLength = 13.0f;
        const float tentHeight = 8.5f;
        const float poleHeight = 9.0f;

        // function for drawing tent panel
        auto drawTentPanel = [&](const glm::vec3& scale, float pitch, float yaw, float roll,
            const glm::vec3& position, const glm::vec4& color, bool withWire = false) {
                SetTransformations(scale, pitch, yaw, roll, position);
                SetShaderColor(color.r, color.g, color.b, color.a);

                SetShaderTexture("canvas");
                SetTextureUVScale(1.5f, 1.5f);

                // canvas material
                m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.90f, 0.88f, 0.82f));
                m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.05f));
                m_pShaderManager->setFloatValue("material.shininess", 4.0f);

                m_basicMeshes->DrawPlaneMesh();

                if (withWire) {
                    glEnable(GL_POLYGON_OFFSET_LINE);
                    glPolygonOffset(-1.0f, -1.0f);
                    glLineWidth(0.8f);
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                    SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f);
                    m_basicMeshes->DrawPlaneMesh();
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                    glDisable(GL_POLYGON_OFFSET_LINE);
                }
            };

        // helper for drawing tent poles (this was a doozy)
        auto drawTentPole = [&](const glm::vec3& position, float height, const glm::vec4& color) {
            SetTransformations(glm::vec3(0.08f, height, 0.08f), 0.0f, 0.0f, 0.0f, position);
            SetShaderColor(color.r, color.g, color.b, color.a);

            SetShaderTexture("bark");
            SetTextureUVScale(1.0f, 2.0f);

            // Wood material for poles
            m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.45f, 0.30f, 0.18f));
            m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.10f));
            m_pShaderManager->setFloatValue("material.shininess", 16.0f);

            m_basicMeshes->DrawCylinderMesh();
            };

        // Tent colors
        const glm::vec4 tentColor = glm::vec4(0.90f, 0.87f, 0.80f, 1.0f);
        const glm::vec4 poleColor = glm::vec4(0.60f, 0.45f, 0.30f, 1.0f);

        // Main pyramid body
        SetTransformations(
            glm::vec3(tentWidth * 0.5f, tentHeight, tentLength * 0.5f),
            0.0f, 300.0f, 0.0f,
            tentCenter + glm::vec3(0.0f, 0.1f, 0.0f)
        );
        SetShaderColor(tentColor.r, tentColor.g, tentColor.b, tentColor.a);
        SetShaderTexture("canvas");
        SetTextureUVScale(4.0f, 4.0f);
        m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.95f, 0.92f, 0.85f));
        m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.15f, 0.12f, 0.08f));
        m_pShaderManager->setFloatValue("material.shininess", 8.0f);

        m_basicMeshes->DrawConeMesh();

        // TENT STAKES AND GUY LINES
        const float stakeRadius = 0.04f;
        const float stakeHeight = 0.3f;
        const float stakeDistance = 4.9f;
        const glm::vec4 stakeColor = glm::vec4(0.35f, 0.25f, 0.15f, 1.0f);
        std::vector<glm::vec3> stakePositions = {
            tentCenter + glm::vec3(5.5f, 0.0f, 0.5f),
            tentCenter + glm::vec3(-1.2f, 0.0f, 4.9f),
            tentCenter + glm::vec3(-4.0f, 0.0f, -2.5f),
            tentCenter + glm::vec3(5.5f, 0.0f, -4.0f),
            tentCenter + glm::vec3(-5.9f, 0.0f, 0.10f)
        };

        // TENT STAKES
        {
            auto drawStake = [&](const glm::vec3& position, int stakeIndex) {
                glm::vec3 directionToTent = glm::normalize(tentCenter - position);
                float yawAngle = glm::degrees(atan2(directionToTent.x, directionToTent.z));
                
                SetTransformations(
                    glm::vec3(stakeRadius, stakeHeight, stakeRadius),
                    0.0f, yawAngle, 0.0f,
                    position
                );
                SetShaderColor(stakeColor.r, stakeColor.g, stakeColor.b, stakeColor.a);
                
                SetShaderTexture("bark");
                SetTextureUVScale(0.3f, 1.0f);
                m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.30f, 0.20f, 0.12f));
                m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.05f));
                m_pShaderManager->setFloatValue("material.shininess", 8.0f);
                m_basicMeshes->DrawCylinderMesh();
            };
            
            for (int i = 0; i < 5; ++i) {
                stakePositions[i].y = stakeHeight * 0.5f;
                drawStake(stakePositions[i], i);
            }
        }

        // GUY LINES (another doozy)
        {
            const float lineWidth = 0.15f;
            const glm::vec4 lineColor = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
            
            glm::vec3 tentPoleTop = tentCenter + glm::vec3(0.0f, poleHeight, 0.0f);
            glm::vec3 stakeTop = stakePositions[0] + glm::vec3(0.0f, stakeHeight * 0.5f, 0.0f);
            
            glm::vec3 lineDir = stakeTop - tentPoleTop;
            float lineLength = glm::length(lineDir);
            glm::vec3 lineCenter = (stakeTop + tentPoleTop) * 0.5f;
            
            DrawRopeBetweenPoints(tentPoleTop, stakeTop, 0.02f, 1.0f);
            
            for (int i = 1; i < 5; ++i) {
                glm::vec3 currentStakeTop = stakePositions[i] + glm::vec3(0.0f, stakeHeight * 0.5f, 0.0f);
                DrawRopeBetweenPoints(tentPoleTop, currentStakeTop, 0.02f, 1.0f);
            }
        }

        // TENT POLE
        drawTentPole(tentCenter + glm::vec3(0.0f, 0.0f, 0.0f), poleHeight, poleColor);

    }
    /******** END TENT OBJECT ********/


    /******** added! BACKPACK OBJECT START ********/
    {
        const glm::vec3 backpackCenter = glm::vec3(-5.5f, 0.0f, 1.0f);
        
        // MAIN BACKPACK BODY
        SetTransformations(
            glm::vec3(1.6f, 2.5f, 0.8f),
            0.0f, 225.0f, 0.0f,
            backpackCenter + glm::vec3(0.0f, 1.25f, 0.0f)
        );

        SetShaderTexture("canvas2");
        SetTextureUVScale(1.0f, 1.0f);

        m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.2f, 0.4f, 0.8f));
        m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.1f, 0.1f, 0.15f));
        m_pShaderManager->setFloatValue("material.shininess", 8.0f);

        m_basicMeshes->DrawBoxMesh();

        // BACKPACK STRAPS
        // Left shoulder strap
        SetTransformations(
            glm::vec3(0.08f, 2.2f, 0.12f),
            0.0f, 225.0f, 0.0f,
            backpackCenter + glm::vec3(0.1f, 1.7f, -0.75f)
        );
        SetShaderTexture("leather");
        SetTextureUVScale(1.0f, 1.0f);
        m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.25f, 0.2f, 0.15f));
        m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.1f, 0.08f, 0.06f));
        m_pShaderManager->setFloatValue("material.shininess", 8.0f);
        m_basicMeshes->DrawBoxMesh();

        // Right shoulder strap
        SetTransformations(
            glm::vec3(0.08f, 2.2f, 0.12f),
            0.0f, 225.0f, 0.0f,
            backpackCenter + glm::vec3(-0.85f, 1.7f, 0.10f)
        );
        SetShaderTexture("leather");
        SetTextureUVScale(1.0f, 1.0f);
        m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.25f, 0.2f, 0.15f));
        m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.1f, 0.08f, 0.06f));
        m_pShaderManager->setFloatValue("material.shininess", 8.0f);
        m_basicMeshes->DrawBoxMesh();

        // BACKPACK TOP FLAP
        SetTransformations(
            glm::vec3(1.6f, 0.15f, 0.8f),      // Same width and depth as main body (1.6f, 0.8f), thin height
            0.0f, 225.0f, 0.0f,                // Same rotation as main body
            backpackCenter + glm::vec3(0.0f, 2.575f, 0.0f) // Positioned directly on top of main body (2.5f + 0.075f)
        );
        SetShaderTexture("tan-leather");  // Tan leather texture for top flap
        SetTextureUVScale(1.0f, 1.0f);
        // Leather material for the top flap
        m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.4f, 0.3f, 0.2f)); // Brown leather color
        m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.2f, 0.15f, 0.1f)); // Leather specular
        m_pShaderManager->setFloatValue("material.shininess", 12.0f); 
        m_basicMeshes->DrawBoxMesh();

    // BACKPACK FRONT FLAP
    SetTransformations(
        glm::vec3(1.3f, 1.0f, 0.02f),      
        0.0f, 225.0f, 0.0f,               
        backpackCenter + glm::vec3(0.3f, 2.0f, 0.27f) 
    );
    SetShaderTexture("tan-leather"); 
    SetTextureUVScale(1.0f, 1.0f);
    
    m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.4f, 0.3f, 0.2f)); 
    m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.2f, 0.15f, 0.1f)); 
    m_basicMeshes->DrawBoxMesh();

    // BACKPACK FRONT FLAP BUCKLE - Rectangular outline at center bottom
    SetTransformations(
        glm::vec3(0.3f, 0.05f, 0.04f),      
        0.0f, 225.0f, 0.0f,                
        backpackCenter + glm::vec3(0.3f, 1.475f, 0.28f) 
    );
    SetShaderColor(1.0f, 0.84f, 0.0f, 1.0f); 
    SetShaderTexture("gold");
    SetTextureUVScale(1.0f, 1.0f);
    m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(1.0f, 0.84f, 0.0f)); 
    m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.1f, 0.08f, 0.06f));
    m_pShaderManager->setFloatValue("material.shininess", 16.0f); 
    m_basicMeshes->DrawBoxMesh();

    // BACKPACK FRONT POCKET
    SetTransformations(
        glm::vec3(1.6f, 1.0f, 0.02f),      
        0.0f, 225.0f, 0.0f,                
        backpackCenter + glm::vec3(0.30f, 0.65f, 0.30f) 
    );
    SetShaderTexture("tan-leather"); // Front flap texture
    SetTextureUVScale(1.0f, 1.0f);
    // Leather material for the front flap
    m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.4f, 0.3f, 0.2f)); 
    m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.2f, 0.15f, 0.1f)); 
    m_pShaderManager->setFloatValue("material.shininess", 12.0f); 
    m_basicMeshes->DrawBoxMesh();
    }

    /******** END BACKPACK OBJECT ********/

    /******** added! PINE TREE OBJECT START ********/
    {
        const glm::vec3 treeCenter = glm::vec3(6.0f, 0.0f, -6.0f);
        const float treeHeight = 16.0f;
        const float trunkHeight = 4.0f;
        const float trunkRadius = 0.6f;
        const float foliageRadius = 3.5f;

        // Helper function for drawing tree trunk
        auto drawTreeTrunk = [&](const glm::vec3& position, float height, float radius) {
            SetTransformations(
                glm::vec3(radius, height, radius),
                0.0f, 0.0f, 0.0f,
                position
            );
            
            SetShaderColor(0.4f, 0.25f, 0.15f, 1.0f);
            SetShaderTexture("bark");
            SetTextureUVScale(1.0f, 3.0f);
            m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.45f, 0.30f, 0.18f));
            m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.08f, 0.06f, 0.04f));
            m_pShaderManager->setFloatValue("material.shininess", 12.0f);
            m_basicMeshes->DrawCylinderMesh();
        };
            
        // Helper function for drawing tree foliage
        auto drawTreeFoliage = [&](const glm::vec3& position, float height, float radius) {
            SetTransformations(
                glm::vec3(radius, height, radius),
                0.0f, 5.0f, 0.0f,
                position
            );
            
            SetShaderColor(0.2f, 0.4f, 0.2f, 1.0f);
            SetShaderTexture("pine-needle");
            SetTextureUVScale(2.0f, 2.0f);
            m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.25f, 0.45f, 0.25f));
            m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.05f, 0.08f, 0.05f));
            m_pShaderManager->setFloatValue("material.shininess", 6.0f);
            m_basicMeshes->DrawConeMesh();
        };

        // TREE TRUNK
        glm::vec3 trunkPos = treeCenter + glm::vec3(0.0f, 0.0f, 0.0f);
        drawTreeTrunk(trunkPos, trunkHeight, trunkRadius);

        // TREE FOLIAGE
        auto drawFoliageWithCap = [&](const glm::vec3& pos, float height, float radius, float capSize) {
            drawTreeFoliage(pos, height, radius);
            
            SetTransformations(
                glm::vec3(capSize * 1.2f, capSize * 1.2f, capSize * 1.2f),
                0.0f, 0.0f, 0.0f,
                pos + glm::vec3(0.0f, height * 0.5f, 0.0f)
            );
            SetShaderColor(0.2f, 0.4f, 0.2f, 1.0f);
            SetShaderTexture("pine-needle");
            SetTextureUVScale(1.0f, 1.0f);
            m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.25f, 0.45f, 0.25f));
            m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.05f, 0.08f, 0.05f));
            m_pShaderManager->setFloatValue("material.shininess", 6.0f);
            m_basicMeshes->DrawSphereMesh();
                
            SetTransformations(
                glm::vec3(radius * 0.8f, height * 0.8f, radius * 0.8f),
                0.0f, 45.0f, 0.0f,
                pos + glm::vec3(0.0f, height * 0.15f, 0.0f)
            );
            SetShaderColor(0.2f, 0.4f, 0.2f, 1.0f);
            SetShaderTexture("pine-needle");
            SetTextureUVScale(1.5f, 1.5f);
            m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.25f, 0.45f, 0.25f));
            m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.05f, 0.08f, 0.05f));
            m_pShaderManager->setFloatValue("material.shininess", 6.0f);
            m_basicMeshes->DrawConeMesh();
            
            SetTransformations(
                glm::vec3(radius * 0.6f, height * 0.6f, radius * 0.6f),
                0.0f, -30.0f, 0.0f,
                pos + glm::vec3(0.0f, height * 0.35f, 0.0f)
            );
            SetShaderColor(0.2f, 0.4f, 0.2f, 1.0f);
            SetShaderTexture("pine-needle");
            SetTextureUVScale(1.5f, 1.5f);
            m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.25f, 0.45f, 0.25f));
            m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.05f, 0.08f, 0.05f));
            m_pShaderManager->setFloatValue("material.shininess", 6.0f);
            m_basicMeshes->DrawConeMesh();
            
            SetTransformations(
                glm::vec3(radius * 0.4f, height * 0.4f, radius * 0.4f),
                0.0f, 60.0f, 0.0f,
                pos + glm::vec3(0.0f, height * 0.5f, 0.0f)
            );
            SetShaderColor(0.2f, 0.4f, 0.2f, 1.0f);
            SetShaderTexture("pine-needle");
            SetTextureUVScale(1.5f, 1.5f);
            m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.25f, 0.45f, 0.25f));
            m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.05f, 0.08f, 0.05f));
            m_pShaderManager->setFloatValue("material.shininess", 6.0f);
            m_basicMeshes->DrawConeMesh();
        };

        // Layer 1 - Bottom
        glm::vec3 pos1 = treeCenter + glm::vec3(0.0f, trunkHeight, 0.0f);
        drawFoliageWithCap(pos1, (treeHeight - trunkHeight) * 0.35f, foliageRadius * 1.0f, foliageRadius * 0.15f);

        // Layer 2 - Second layer
        glm::vec3 pos2 = treeCenter + glm::vec3(0.0f, trunkHeight + (treeHeight - trunkHeight) * 0.12f, 0.0f);
        drawFoliageWithCap(pos2, (treeHeight - trunkHeight) * 0.3f, foliageRadius * 0.75f, foliageRadius * 0.12f);

        // Layer 3 - Third layer
        glm::vec3 pos3 = treeCenter + glm::vec3(0.0f, trunkHeight + (treeHeight - trunkHeight) * 0.24f, 0.0f);
        drawFoliageWithCap(pos3, (treeHeight - trunkHeight) * 0.25f, foliageRadius * 0.55f, foliageRadius * 0.10f);

        // Layer 4 - Top layer
        glm::vec3 pos4 = treeCenter + glm::vec3(0.0f, trunkHeight + (treeHeight - trunkHeight) * 0.36f, 0.0f);
        drawFoliageWithCap(pos4, (treeHeight - trunkHeight) * 0.2f, foliageRadius * 0.35f, foliageRadius * 0.08f);

    }
    /******** END PINE TREE OBJECT ********/

    /*************** MOON OBJECT START ******************/
    {
        const glm::vec3 moonPos = glm::vec3(0.5f, 10.2f, -6.0f);
        const float moonRad = 0.75f;

        auto drawMoon = [&](const glm::vec3& p, float radius) {
            m_pShaderManager->setBoolValue(g_UseLightingName, false);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(GL_FALSE);

            SetTransformations(glm::vec3(radius, radius, radius), 0.0f, 0.0f, 0.0f, p);
            SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
            SetTextureUVScale(1.0f, 1.0f);
            SetShaderTexture("moon");
            m_basicMeshes->DrawSphereMesh();

            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
            m_pShaderManager->setBoolValue(g_UseLightingName, true);
        };

        drawMoon(moonPos, moonRad);

        const glm::vec3 sceneCenter = glm::vec3(0.0f, 0.8f, 0.0f);
        glm::vec3 dirLightToScene = glm::normalize(sceneCenter - moonPos);
        m_pShaderManager->setVec3Value("directionalLight.direction", dirLightToScene);
        m_pShaderManager->setVec3Value("directionalLight.ambient", glm::vec3(0.03f, 0.04f, 0.06f));
        m_pShaderManager->setVec3Value("directionalLight.diffuse", glm::vec3(0.22f, 0.24f, 0.30f));
        m_pShaderManager->setVec3Value("directionalLight.specular", glm::vec3(0.18f, 0.20f, 0.24f));
        m_pShaderManager->setBoolValue("directionalLight.bActive", true);
    }
    /******** END MOON OBJECT ********/

}