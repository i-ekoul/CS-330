///////////////////////////////////////////////////////////////////////////////
// maincode.cpp
// ============
// gets called when application is launched - initializes GLEW, GLFW
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include <iostream>         // error handling and output
#include <cstdlib>          // EXIT_FAILURE
#include <limits>           // std::numeric_limits for ray-AABB intersection
#include <cmath>            // std::abs for EPS comparisons

#include <GL/glew.h>        // GLEW library
#include "GLFW/glfw3.h"     // GLFW library

// GLM Math Header inclusions
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "SceneManager.h"
#include "ViewManager.h"
#include "ShapeMeshes.h"
#include "ShaderManager.h"

// ============================================================================
// Milestone Two: Picking Enhancement
// ============================================================================
// Minimal local types for ray-AABB intersection testing
// ============================================================================

// Ray structure: origin point and normalized direction vector
// NOTE: The direction vector MUST be normalized by the caller before use
struct Ray 
{ 
	glm::vec3 origin;   // Ray origin point in world space
	glm::vec3 dir;      // Ray direction (must be normalized)
};

// Axis-Aligned Bounding Box structure: minimum and maximum corner points
struct AABB 
{ 
	glm::vec3 min;      // Minimum corner (all components <= max)
	glm::vec3 max;      // Maximum corner (all components >= min)
};

// ============================================================================
// Ray-AABB Intersection Helper (Slab Method)
// ============================================================================
// Implements robust slab-based ray-AABB intersection with EPS guards to handle
// near-zero direction components safely (parallel slab case).
//
// Parameters:
//   r: Ray with normalized direction vector
//   box: Axis-aligned bounding box to test against
//   tNear: Output parameter for the nearest intersection distance (if hit)
//
// Returns:
//   true if ray intersects the AABB, false otherwise
//
// Behavior:
//   - If ray origin is inside the AABB, treats as hit and sets tNear = 0
//   - Otherwise computes intersection t using slab logic; returns nearest valid hit
//   - Handles near-zero direction components safely using EPS guards
// ============================================================================
static bool rayAabbSlab(const Ray& r, const AABB& box, float& tNear)
{
	const float EPS = 1e-6f;  // Epsilon for near-zero comparisons
	
	// Check if ray origin is inside the AABB (treat as immediate hit)
	bool inside = (r.origin.x >= box.min.x && r.origin.x <= box.max.x &&
	               r.origin.y >= box.min.y && r.origin.y <= box.max.y &&
	               r.origin.z >= box.min.z && r.origin.z <= box.max.z);

	if (inside)
	{
		tNear = 0.0f;  // Ray origin is inside, treat as immediate hit
		return true;
	}

	float tMin = 0.0f;        // Minimum t (entry point)
	float tMax = std::numeric_limits<float>::max();  // Maximum t (exit point)

	// Test each axis-aligned slab (X, Y, Z)
	for (int axis = 0; axis < 3; ++axis)
	{
		// Handle parallel slab case (near-zero direction component)
		if (std::abs(r.dir[axis]) < EPS)
		{
			// Ray is parallel to this axis; check if origin is within bounds
			if (r.origin[axis] < box.min[axis] || r.origin[axis] > box.max[axis])
			{
				return false;  // Ray misses the AABB on this axis
			}
			// Origin is within bounds on this axis, continue to next axis
			continue;
		}

		// Non-parallel case: compute intersection with slab planes
		float invDir = 1.0f / r.dir[axis];
		float t0 = (box.min[axis] - r.origin[axis]) * invDir;
		float t1 = (box.max[axis] - r.origin[axis]) * invDir;

		// Handle negative direction (swap t0 and t1)
		if (invDir < 0.0f)
		{
			float temp = t0;
			t0 = t1;
			t1 = temp;
		}

		// Update tMin and tMax (intersection of all slabs)
		tMin = (t0 > tMin) ? t0 : tMin;
		tMax = (t1 < tMax) ? t1 : tMax;

		// Early exit if no intersection possible
		if (tMin > tMax)
		{
			return false;
		}
	}

	// Valid intersection found
	if (tMin >= 0.0f)
	{
		tNear = tMin;  // Nearest intersection point
		return true;
	}

	// Ray starts behind the AABB but may still intersect
	if (tMax >= 0.0f)
	{
		tNear = tMax;  // Use exit point
		return true;
	}

	return false;  // No valid intersection
}

// ============================================================================
// Object Picking Entry Point
// ============================================================================
// Determines which object (if any) is intersected by the given ray.
// Currently implements naive O(n) AABB checks against a deterministic list.
//
// Parameters:
//   r: Ray with normalized direction vector
//
// Returns:
//   Object ID of the closest hit (smallest non-negative tNear), or -1 if no hit
//
// Implementation Notes:
//   - Iterates through a deterministic list of AABBs/IDs
//   - Returns the ID of the closest hit (smallest tNear >= 0)
//   - Deterministic ordering ensures consistent results
//
// Future Enhancement (Broad-Phase):
//   The current implementation uses naive O(n) AABB checks. A planned broad-phase
//   optimization will use a uniform grid spatial acceleration structure:
//
//   UNIFORM GRID APPROACH:
//   1. Cell Size: Determine optimal cell size based on scene bounds and object density
//      (e.g., cellSize = sceneBounds / gridResolution, where gridResolution ~ sqrt(n))
//   2. Scene Bounds: Compute overall AABB encompassing all scene objects
//   3. Object-to-Cell Mapping: For each object, determine which grid cells its AABB overlaps
//      - Store object IDs in each overlapping cell's list
//   4. 3D-DDA Traversal: Use 3D Digital Differential Analyzer to step through grid cells
//      along the ray direction:
//      - Start at cell containing ray origin
//      - Compute t values for crossing each cell boundary (X, Y, Z planes)
//      - Step to next cell using minimum t (Bresenham-like algorithm)
//      - Test objects in each traversed cell
//      - Early exit on first hit if only nearest hit needed
//   5. Benefits: Reduces intersection tests from O(n) to O(sqrt(n)) average case
//
//   The broad-phase can remain "planned" as comments, but the current pickObjectId
//   must work via naive O(n) AABB checks.
// ============================================================================
static int pickObjectId(const Ray& r)
{
	// Deterministic list of scene objects with their AABBs and IDs
	// NOTE: These bounds should match the actual scene objects for accurate picking
	// For this milestone, we use example bounds that represent typical scene objects
	
	struct ObjectBounds
	{
		int id;
		AABB bounds;
	};

	// Deterministic list of objects (ordered by ID for consistency)
	// Example bounds representing a campfire scene with multiple objects
	const ObjectBounds objects[] = {
		{ 0, { glm::vec3(-1.0f, 0.0f, -1.0f), glm::vec3(1.0f, 2.0f, 1.0f) } },   // Object 0: Large structure (e.g., campfire)
		{ 1, { glm::vec3(2.0f, 0.0f, -0.5f), glm::vec3(3.0f, 1.5f, 0.5f) } },   // Object 1: Medium object (e.g., backpack)
		{ 2, { glm::vec3(-2.0f, 0.0f, 1.0f), glm::vec3(-1.5f, 0.5f, 2.0f) } },  // Object 2: Small object (e.g., log)
		{ 3, { glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.5f, 1.0f, 2.5f) } },    // Object 3: Medium object (e.g., tent)
		{ 4, { glm::vec3(-0.5f, 0.0f, -2.0f), glm::vec3(0.5f, 0.3f, -1.5f) } },  // Object 4: Small object (e.g., ground item)
	};

	const int numObjects = sizeof(objects) / sizeof(objects[0]);

	int closestId = -1;
	float closestT = std::numeric_limits<float>::max();

	// Naive O(n) intersection test against all objects
	for (int i = 0; i < numObjects; ++i)
	{
		float tNear;
		if (rayAabbSlab(r, objects[i].bounds, tNear))
		{
			// Valid hit found; check if it's closer than previous hits
			if (tNear >= 0.0f && tNear < closestT)
			{
				closestT = tNear;
				closestId = objects[i].id;
			}
		}
	}

	return closestId;  // Returns -1 if no hit, otherwise ID of closest object
}

// ============================================================================
// Evidence Hooks: Deterministic Sample Rays (Debug/Verification)
// ============================================================================
// These sample rays demonstrate the picking functionality:
// - Ray 1: Should hit a known AABB and return its ID
// - Ray 2: Should miss all AABBs and return -1
//
// NOTE: These are kept as comments to avoid changing runtime behavior unless
// there is already a debug print pathway. They serve as verification examples.
// ============================================================================
/*
// Evidence Hook 1: Ray that should hit Object 0 (campfire structure)
// Ray originates at (0, 1, -2) pointing toward Object 0's center
static void testRayHit()
{
	Ray testRay;
	testRay.origin = glm::vec3(0.0f, 1.0f, -2.0f);
	testRay.dir = glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f));  // Points toward Object 0
	
	int hitId = pickObjectId(testRay);
	// Expected: hitId == 0 (hits Object 0's AABB)
	// If debug print available: std::cout << "Test Ray Hit: Object ID = " << hitId << std::endl;
}

// Evidence Hook 2: Ray that should miss all objects
// Ray originates far away pointing in a direction that misses all AABBs
static void testRayMiss()
{
	Ray testRay;
	testRay.origin = glm::vec3(10.0f, 10.0f, 10.0f);
	testRay.dir = glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f));  // Points away from scene
	
	int hitId = pickObjectId(testRay);
	// Expected: hitId == -1 (no intersection)
	// If debug print available: std::cout << "Test Ray Miss: Object ID = " << hitId << std::endl;
}
*/

// Namespace for declaring global variables
namespace
{
	// Macro for window title
	const char* const WINDOW_TITLE = "7-1 FinalProject and Milestones"; 

	// Main GLFW window
	GLFWwindow* g_Window = nullptr;

	// scene manager object for managing the 3D scene prepare and render
	SceneManager* g_SceneManager = nullptr;
	// shader manager object for dynamic interaction with the shader code
	ShaderManager* g_ShaderManager = nullptr;
	// view manager object for managing the 3D view setup and projection to 2D
	ViewManager* g_ViewManager = nullptr;
}

// Function declarations - all functions that are called manually
// need to be pre-declared at the beginning of the source code.
bool InitializeGLFW();
bool InitializeGLEW();


/***********************************************************
 *  main(int, char*)
 *
 *  This function gets called after the application has been
 *  launched.
 ***********************************************************/
int main(int argc, char* argv[])
{
	// if GLFW fails initialization, then terminate the application
	if (InitializeGLFW() == false)
	{
		return(EXIT_FAILURE);
	}

	// try to create a new shader manager object
	g_ShaderManager = new ShaderManager();
	// try to create a new view manager object
	g_ViewManager = new ViewManager(
		g_ShaderManager);

	// try to create the main display window
	g_Window = g_ViewManager->CreateDisplayWindow(WINDOW_TITLE);

	// if GLEW fails initialization, then terminate the application
	if (InitializeGLEW() == false)
	{
		return(EXIT_FAILURE);
	}

	// load the shader code from the external GLSL files
	g_ShaderManager->LoadShaders(
		"shaders/vertexShader.glsl",
		"shaders/fragmentShader.glsl");
	g_ShaderManager->use();

	// try to create a new scene manager object and prepare the 3D scene
	g_SceneManager = new SceneManager(g_ShaderManager);
	g_SceneManager->PrepareScene();

	// loop will keep running until the application is closed 
	// or until an error has occurred
	while (!glfwWindowShouldClose(g_Window))
	{
		// Enable z-depth
		glEnable(GL_DEPTH_TEST);

		// Clear the frame and z buffers
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// convert from 3D object space to 2D view
		g_ViewManager->PrepareSceneView();

		// refresh the 3D scene
		g_SceneManager->RenderScene();


		// Flips the the back buffer with the front buffer every frame.
		glfwSwapBuffers(g_Window);

		// query the latest GLFW events
		glfwPollEvents();
	}

	// clear the allocated manager objects from memory
	if (NULL != g_SceneManager)
	{
		delete g_SceneManager;
		g_SceneManager = NULL;
	}
	if (NULL != g_ViewManager)
	{
		delete g_ViewManager;
		g_ViewManager = NULL;
	}
	if (NULL != g_ShaderManager)
	{
		delete g_ShaderManager;
		g_ShaderManager = NULL;
	}

	// Terminates the program successfully
	exit(EXIT_SUCCESS); 
}

/***********************************************************
 *	InitializeGLFW()
 * 
 *  This function is used to initialize the GLFW library.   
 ***********************************************************/
bool InitializeGLFW()
{
	// GLFW: initialize and configure library
	// --------------------------------------
	glfwInit();

#ifdef __APPLE__
	// set the version of OpenGL and profile to use
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
	// set the version of OpenGL and profile to use
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
	// GLFW: end -------------------------------

	return(true);
}

/***********************************************************
 *	InitializeGLEW()
 *
 *  This function is used to initialize the GLEW library.
 ***********************************************************/
bool InitializeGLEW()
{
	// GLEW: initialize
	// -----------------------------------------
	GLenum GLEWInitResult = GLEW_OK;

	// try to initialize the GLEW library
	GLEWInitResult = glewInit();
	if (GLEW_OK != GLEWInitResult)
	{
		std::cerr << glewGetErrorString(GLEWInitResult) << std::endl;
		return false;
	}
	// GLEW: end -------------------------------

	// Displays a successful OpenGL initialization message
	std::cout << "INFO: OpenGL Successfully Initialized\n";
	std::cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << "\n" << std::endl;

	return(true);
}