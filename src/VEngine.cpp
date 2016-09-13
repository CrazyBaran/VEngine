﻿#include "VEngine.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>
#include "OctreeChunk.h"

namespace vengine {

bool debugDraw = false;



VEngine::VEngine() {}
VEngine::~VEngine()
{
	Destroy();
}

int
VEngine::Init(const std::string& gameTitle)
{
	_gameTitle = gameTitle;
	int error;

	error = InitGLFWLibrary();
	if (error) {
		printf("Cannot initialize GLFW library. Error code: %d", error);
		goto glfw_err;
	}

	error = InitFileManagers();
	if (error) {
		printf("Cannot initialize file managers. Error code: %d", error);
		goto fileman_err;
	}

	error = InitWindow();
	if (error) {
		printf("Cannot initialize file managers. Error code: %d", error);
		goto callbacks_err;
	}

	error = InitGLFWCallbacks();
	if (error) {
		printf("Cannot assign GLFW callbacks. Error code: %d", error);
		goto callbacks_err;
	}

	error = InitOtherManagers();
	if (error) {
		printf("Cannot initialize other managers. Error code: %d", error);
		goto callbacks_err;
	}

	error = InitResourceManagers();
	if (error) {
		printf("Cannot initialize resource managers. Error code: %d", error);
		goto resman_err;
	}

	error = InitLocalResources();
	if (error) {
		printf("Cannot initialize local resources. Error code: %d", error);
		goto locres_err;
		;
	}

	return 0;


locres_err:
	DestroyResourceManagers();
resman_err:
	DestroyOtherManagers();
callbacks_err:
	DestroyFileManagers();
fileman_err:
	DestroyGLFWLibrary();
glfw_err:
	return error;
}

int
VEngine::InitLocalResources()
{
	_renderer.Init();

	return 0;
}

int
VEngine::InitFileManagers()
{
	try {
		new ShaderFileManager;
	}
	catch (std::bad_alloc err) {
		DestroyFileManagers();
		return VE_EMEM;
	}

	return 0;
}
int
VEngine::InitOtherManagers()
{
	return 0;
}
int
VEngine::InitResourceManagers()
{
	try {
		new ShaderManager;
		new GlProgramManager;
		new GlPipelineManager;
		new TextureManager;
		new MeshManager;
		new VoxelArrayManager;
	}
	catch (std::bad_alloc err) {
		DestroyResourceManagers();
		return VE_EMEM;
	}

	return 0;
}
int
VEngine::InitGLFWLibrary()
{
	if (!glfwInit())
		return VE_FAULT;
	return 0;
}
int
VEngine::InitWindow()
{
	//Read resolution from config file
	Window::Init(1366, 768, _gameTitle.c_str());
	if (!Window::CreateWindowed())
		return VE_FAULT;
	Window::MakeActiveContext();
	return 0;
}
int
VEngine::InitGLFWCallbacks()
{
	glfwSetErrorCallback(ErrorHandler);
	return 0;
}

int
VEngine::Load()
{
	int error;
	error = LoadInput();
	if (error) {
		fprintf(stderr, "Failed to load Input! Error: %d\n", error);
		return error;
	}
	error = LoadResources();
	if (error) {
		fprintf(stderr, "Failed to load resources! Error: %d\n", error);
		return error;
	}
	return 0;
}

int
VEngine::LoadInput()
{
	Input::Init(keyNames, keyCodes, bindNum);
	Input::SetMode(Input::KEY_MODE);
	Input::DisableCursor();

	return 0;
}


int
VEngine::LoadResources()
{
	int error;
	error = LoadShaders();
	if (error)
		return error;
	error = LoadTextures();
	if (error)
		return error;

	LoadWorld();

	return 0;
}

int
VEngine::LoadShaders()
{
	
	int error;

	error = _renderer.AddShader("VertexSimple", "Shaders/Simple.vert", Shader::VERTEX, Renderer::STANDARD);
	if (error) {
		std::cout << "Adding failed for VertexSimple - Shaders/Simple.vert" << std::endl;
		return error;
	}

	error = _renderer.AddShader("FragSimple", "Shaders/Simple.frag", Shader::FRAGMENT, Renderer::STANDARD);
	if (error) {
		std::cout << "Adding failed for FragSimple - Shaders/Simple.frag" << std::endl;
		return error;
	}

	error = _renderer.AddShader("VertexVoxel", "Shaders/Voxel.vert", Shader::VERTEX, Renderer::VOXEL);
	if (error) {
		std::cout << "Adding failed for VertexSimple - Shaders/Simple.vert" << std::endl;
		return error;
	}

	error = _renderer.AddShader("FragVoxel", "Shaders/Voxel.frag", Shader::FRAGMENT, Renderer::VOXEL);
	if (error) {
		std::cout << "Adding failed for FragSimple - Shaders/Simple.frag" << std::endl;
		return error;
	}

	return 0;
}

int 
VEngine::LoadTextures()
{
	unsigned int tex = textureManager.GetTexture("WoodOld");
	int rc = textureManager.LoadTexture(tex, "Textures/wood.jpg");
	if (rc) {
		std::cout << "Failed to load texture wood.jpg" << std::endl;
		return rc;
	}
	
	tex = textureManager.GetTexture("Atlas");
	rc = textureManager.LoadTexture(tex, "Textures/tex.tga");
	if (rc) {
		std::cout << "Failed to load texture tex.tga" << std::endl;
		return rc;
	}

	VoxelMesh::SetAtlas(tex);
	_renderer.SetAtlasTileSize(1.0f / Voxel::texsPerRow);
	return 0;
}

void 
VEngine::LoadWorld()
{
	_renderer.SetClearColor(Vector3(135 / 255.0f, 191 / 255.0f, 255/255.0f));

	_world = new World("World");
	_world->Rename("World");


	VoxelMesh* mesh = new VoxelMesh;
	mesh->Init("Sword");

	int vaSword = voxelArrayManager.GetVoxelArray("VoxelSword");
	voxelArrayManager.SetDimension(vaSword, swordSize.x, swordSize.y, swordSize.z);
	voxelArrayManager.SetVoxels(vaSword, swordVoxels);
	voxelArrayManager.SetVoxelSize(vaSword, 0.1f);
	voxelArrayManager.GenerateMesh(vaSword, mesh);
	unsigned int swordMesh = meshManager.AddMesh(mesh);

	mesh = new VoxelMesh;
	mesh->Init("Cube");
	int cubeVox = voxelArrayManager.GetVoxelArray("Cube");
	voxelArrayManager.SetDimension(cubeVox, cubeSize.x, cubeSize.y, cubeSize.z);
	voxelArrayManager.SetVoxels(cubeVox, cube);
	voxelArrayManager.SetVoxelSize(cubeVox, 1.0f);
	voxelArrayManager.GenerateMesh(cubeVox, mesh);
	unsigned int cubeMesh = meshManager.AddMesh(mesh);
	
	Transform transformMain, transformChild, transformPlayer;
	transformMain.SetPosition(Vector3(0.0f, 30.0f, 0.0f));
	transformPlayer.SetPosition(Vector3(4.0f, 50.0f, 4.0f));
	transformChild.Set(Vector3(0.0f, 1.0f, 0.0f), Vector3(0.5f, 0.5f, 0.5f), Quaternion().SetRotationX(90.0f));

	PlayerController* player = new PlayerController;
	CameraFPP* camera = new CameraFPP;

	player->SetCamera(camera);
	_renderer.SetActiveCamera(camera);
	player->GetCollider().SetDimension(Vector3(0.5f, 1.5f, 0.5f));
	player->AttachTo(_world);
	player->SetTransform(transformPlayer);
	player->SetOctree(&_octree);

	PhysicalObject* sword = new PhysicalObject("Sword");
	sword->SetMesh(swordMesh);
	sword->AttachTo(_world);
	sword->SetTransform(transformMain);
	sword->GetCollider().SetPosition(transformMain.GetPosition());
	
	int x, y, z;
	voxelArrayManager.GetDimension(vaSword, &x, &y, &z);
	sword->GetCollider().SetDimension(Vector3((float)x, (float)y, (float)z) * voxelArrayManager.GetVoxelSize(vaSword));

	_octree.Add(sword);
	_octree.Add(player);

	TerrainGenerator terrainGen(0, 5, 1);
	terrainGen.SetSmoothness(256);
	terrainGen.SetDetails(1);
	terrainGen.SetSpread(32);
	terrainGen.SetSeed(58230947u);

	for (int z = 0; z < 16; ++z) {
		for (int y = 0; y < 16; ++y) {
			for (int x = 0; x < 16; ++x) {
				Chunk* chunk = new Chunk(Vector3((x - 8.0f) * 16.0f, (y - 8.0f) * 16.0f, (z - 8.0f) * 16.0f));
				if(terrainGen.GetChunk(chunk))
					_octree.Add(chunk);
			}
		}
	}

	/*
	srand(1299);
	for (int i = 0; i < 10; ++i) {
		Vector3 pos((float)(rand() % 1000), (float)(rand() % 100) + 200, (float)(rand() % 1000));
		pos -= 50.0f;
		pos *= 0.1f;
		Transform transf;
		transf.SetPosition(pos);

		PhysicalObject* swordChild = (PhysicalObject *)GameObject::Instantiate(sword);
		swordChild->AttachTo(_world);
		swordChild->SetTransform(transf);

		_octree.Add(swordChild);
	}
	*/
	_octree.SetBoundingArea(BoundingBox(Vector3::zeroes, Vector3(16 * 16.0f)));

	_renderer.SetAmbientLight(Vector3(1.0f, 1.0f, 1.0f), 0.5f);
	_renderer.SetGlobalLightDir(Vector3(2.0f, 1.0f, 0.5f));
}

void
VEngine::Destroy()
{
	DestroyWorld();
	DestroyFileManagers();
	DestroyResourceManagers();
	//DestroyGLFWLibrary();
}

void
VEngine::DestroyWorld()
{
	delete _world;
}

void
VEngine::DestroyFileManagers()
{
	delete shaderFileManager.GetSingletonPointer();
}
void
VEngine::DestroyResourceManagers()
{
	delete pipelineManager.GetSingletonPointer();
	delete programManager.GetSingletonPointer();
	delete shaderManager.GetSingletonPointer();
	delete textureManager.GetSingletonPointer();
	delete meshManager.GetSingletonPointer();
	delete voxelArrayManager.GetSingletonPointer();
}
void
VEngine::DestroyOtherManagers()
{

}

void
VEngine::DestroyGLFWLibrary()
{
	Window::Destroy();
	glfwTerminate();
}

Renderer& 
VEngine::GetRenderer()
{
	return _renderer;
}



void
VEngine::Run()
{
	_world->Init();
	_octree.UpdateTree();
	_octree.Update();

	Time::Update();
	while (Window::IsOpened()) {
		Time::Update();
		Input::UpdateMouseOffset();


		_world->Update();
		_world->LateUpdate();

		_octree.UpdateTree();
		_octree.Update();

		Draw();
		_octree.Draw(&_renderer);
		_world->Draw(&_renderer);
		_world->LateDraw(&_renderer);

		if (debugDraw) {
			_octree.DrawDebug(&_renderer);
		}

		Input::UpdateInput();
		Window::HandleWindow();

		std::cout << "\rFPS: " << 1.0f / Time::DeltaTime() << "\tPosition: " << _renderer.GetActiveCamera()->GetPosition();

		if (Input::IsPressed("Exit")) {
			Window::Close();
		}
	}
}

void
VEngine::Draw()
{
	_renderer.ClearBuffers();
	_renderer.UpdateProjection();
	_renderer.UpdateView();
}

void
VEngine::ErrorHandler(int error, const char* description)
{
	fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

}