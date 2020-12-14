#include <stdio.h>

#include "Compositor/OgreCompositorManager2.h"
#include "OgreArchiveManager.h"
#include "OgreCamera.h"
#include "OgreConfigFile.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsSamplerblock.h"
#include "OgreHlmsUnlit.h"
#include "OgreItem.h"
#include "OgreMesh.h"
#include "OgreMesh2.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreTextureFilters.h"
#include "OgreTextureGpuManager.h"
#include "OgreWindow.h"
#include "OgreWindowEventUtilities.h"

Ogre::Root* gOgreRoot = nullptr;

static Ogre::SceneManager* gSceneManager = nullptr;

static Ogre::Window* gOgreWindow = nullptr;

static bool gGraphicsIntialized = false;

static bool g_ogre_window_should_quit = false;

class MyWindowEventListener : public Ogre::WindowEventListener
{
public:
	virtual void windowClosed(Ogre::Window* window)
	{
		printf("Window closed!\n");
		g_ogre_window_should_quit = true;
	}
};

MyWindowEventListener g_myWindowEventListener;

static void registerHlms()
{
	Ogre::String resourcePath = "data/";

	Ogre::ConfigFile config;
	config.load((resourcePath + "resources2.cfg"));

	Ogre::String rootHlmsFolder =
	    (resourcePath + config.getSetting("DoNotUseAsResource", "Hlms", "Hlms"));

	if (rootHlmsFolder.empty())
	{
		rootHlmsFolder = "./";
	}
	else if ((*(rootHlmsFolder.end() - 1)) != '/')
	{
		rootHlmsFolder = (rootHlmsFolder + "/");
	}

	Ogre::HlmsUnlit* hlmsUnlit = nullptr;
	Ogre::HlmsPbs* hlmsPbs = nullptr;
	Ogre::String mainFolderPath;
	Ogre::StringVector libraryFoldersPaths;
	Ogre::StringVector::const_iterator libraryFolderPathIt;
	Ogre::StringVector::const_iterator libraryFolderPathEn;
	Ogre::ArchiveManager& archiveManager = Ogre::ArchiveManager::getSingleton();

	// Unlit
	{
		Ogre::HlmsUnlit::getDefaultPaths(mainFolderPath, libraryFoldersPaths);
		Ogre::Archive* archiveUnlit =
		    archiveManager.load((rootHlmsFolder + mainFolderPath), "FileSystem", true);
		Ogre::ArchiveVec archiveUnlitLibraryFolders;
		libraryFolderPathIt = libraryFoldersPaths.begin();
		libraryFolderPathEn = libraryFoldersPaths.end();
		while (libraryFolderPathIt != libraryFolderPathEn)
		{
			Ogre::Archive* archiveLibrary =
			    archiveManager.load((rootHlmsFolder + (*libraryFolderPathIt)), "FileSystem", true);
			archiveUnlitLibraryFolders.push_back(archiveLibrary);
			++libraryFolderPathIt;
		}
		hlmsUnlit = OGRE_NEW Ogre::HlmsUnlit(archiveUnlit, (&archiveUnlitLibraryFolders));
		Ogre::Root::getSingleton().getHlmsManager()->registerHlms(hlmsUnlit);
	}

	// PBS (lit)
	{
		Ogre::HlmsPbs::getDefaultPaths(mainFolderPath, libraryFoldersPaths);
		Ogre::Archive* archivePbs =
		    archiveManager.load((rootHlmsFolder + mainFolderPath), "FileSystem", true);
		Ogre::ArchiveVec archivePbsLibraryFolders;
		libraryFolderPathIt = libraryFoldersPaths.begin();
		libraryFolderPathEn = libraryFoldersPaths.end();
		while (libraryFolderPathIt != libraryFolderPathEn)
		{
			Ogre::Archive* archiveLibrary =
			    archiveManager.load((rootHlmsFolder + (*libraryFolderPathIt)), "FileSystem", true);
			archivePbsLibraryFolders.push_back(archiveLibrary);
			++libraryFolderPathIt;
		}
		hlmsPbs = OGRE_NEW Ogre::HlmsPbs(archivePbs, (&archivePbsLibraryFolders));
		Ogre::Root::getSingleton().getHlmsManager()->registerHlms(hlmsPbs);
	}

	Ogre::RenderSystem* renderSystem = Ogre::Root::getSingletonPtr()->getRenderSystem();
	if (renderSystem->getName() == "Direct3D11 Rendering Subsystem")
	{
		bool supportsNoOverwriteOnTextureBuffers;
		renderSystem->getCustomAttribute("MapNoOverwriteOnDynamicBufferSRV",
		                                 (&supportsNoOverwriteOnTextureBuffers));
		if (!(supportsNoOverwriteOnTextureBuffers))
		{
			hlmsPbs->setTextureBufferDefaultSize((512 * 1024));
			hlmsUnlit->setTextureBufferDefaultSize((512 * 1024));
		}
	}
}

bool ogreInitializeInternal(bool useCurrentWindow)
{
	const Ogre::String pluginsFolder = "./data/";
	const Ogre::String writeAccessFolder = "./output/";
	const char* pluginsFile = "plugins_d.cfg";

	gOgreRoot = OGRE_NEW Ogre::Root((pluginsFolder + pluginsFile), (writeAccessFolder + "ogre.cfg"),
	                                (writeAccessFolder + "Ogre.log"));

	// This allows the user to configure the graphics. It's damn annoying during dev though
	// if (!root->showConfigDialog())
		// return false;

	Ogre::RenderSystem* renderSystem =
	    gOgreRoot->getRenderSystemByName("OpenGL 3+ Rendering Subsystem");
	if (!(renderSystem))
	{
		printf("Render system not found!\n");
		return false;
	}
	renderSystem->setConfigOption("Full Screen", "No");
	renderSystem->setConfigOption("Video Mode", "1920 x 1080");
	renderSystem->setConfigOption("sRGB Gamma Conversion", "Yes");
	gOgreRoot->setRenderSystem(renderSystem);

	gOgreWindow = gOgreRoot->initialise(!(useCurrentWindow), "OgreStartProject");
	if (useCurrentWindow)
	{
		Ogre::NameValuePairList windowSettings;
		windowSettings["currentGLDrawable"] = Ogre::String("True");
		windowSettings["currentGLContext"] = Ogre::String("True");
		int winWidth = 1920;
		int winHeight = 1080;
		gOgreWindow = gOgreRoot->createRenderWindow("OgreStartProject", winWidth, winHeight, false,
		                                            (&windowSettings));
	}

	registerHlms();

	const size_t numThreads = 1u;
	gSceneManager = gOgreRoot->createSceneManager(Ogre::ST_GENERIC, numThreads, "SceneManager");

	Ogre::Camera* camera = gSceneManager->createCamera("Main Camera");
	{
		camera->setPosition(Ogre::Vector3(0, 5, 15));
		camera->lookAt(Ogre::Vector3(0, 0, 0));
		camera->setNearClipDistance(0.2f);
		camera->setFarClipDistance(1000.0f);
		camera->setAutoAspectRatio(true);
	}

	Ogre::CompositorManager2* compositorManager = gOgreRoot->getCompositorManager2();
	const Ogre::String workspaceName = "Main Workspace";
	const Ogre::ColourValue backgroundColour(0.0302f, 0.03f, 0.03f);
	compositorManager->createBasicWorkspaceDef(workspaceName, backgroundColour, Ogre::IdString());
	compositorManager->addWorkspace(gSceneManager, gOgreWindow->getTexture(), camera, workspaceName,
	                                true);

	Ogre::ResourceGroupManager& resourceGroupManager = Ogre::ResourceGroupManager::getSingleton();
	resourceGroupManager.addResourceLocation("data/Models", "FileSystem", "Models");
	resourceGroupManager.addResourceLocation("data/Materials/Textures", "FileSystem", "Textures");

	{
		resourceGroupManager.createResourceGroup("Materials");
		resourceGroupManager.addResourceLocation("data/Materials", "FileSystem", "Materials");
		resourceGroupManager.initialiseResourceGroup("Materials", true);
		resourceGroupManager.loadResourceGroup("Materials");
	}

	if (!useCurrentWindow)
	{
		Ogre::WindowEventUtilities::addWindowEventListener(gOgreWindow, &g_myWindowEventListener);
	}

	gGraphicsIntialized = true;
	return true;
}

void ogreShutdown()
{
	Ogre::WindowEventUtilities::removeWindowEventListener(gOgreWindow, &g_myWindowEventListener);
	OGRE_DELETE(gOgreRoot);
	gOgreRoot = nullptr;
}

Ogre::SceneManager* ogreGetSceneManager()
{
	return gSceneManager;
}

//
// Interface
//

bool ogreInitialize()
{
	return ogreInitializeInternal(false);
}

// Assumes an already created SDL window with OpenGL context
bool ogreInitializeSdl()
{
	return ogreInitializeInternal(true);
}

bool ogreRenderFrame()
{
	return gOgreRoot->renderOneFrame();
}

// Only necessary for old V1 models. You should try not to use this
Ogre::Item* ogreLoadMeshFromV1(const char* name)
{
	Ogre::SceneManager* sceneManager = ogreGetSceneManager();
	Ogre::v1::MeshPtr meshV1 = Ogre::v1::MeshManager::getSingleton().load(
	    name, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
	    Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);
	bool halfPosition = true;
	bool halfUVs = true;
	bool useQTangents = true;
	const Ogre::String meshNameV2 = (name + Ogre::String(" Imported"));
	Ogre::MeshPtr v2Mesh = Ogre::MeshManager::getSingleton().createByImportingV1(
	    meshNameV2, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, meshV1.get(),
	    halfPosition, halfUVs, useQTangents);
	meshV1->unload();
	Ogre::Item* item = sceneManager->createItem(
	    meshNameV2, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
	    Ogre::SCENE_DYNAMIC);
	return item;
}

// V2 mesh loading
Ogre::Item* ogreLoadMesh(const char* name)
{
	Ogre::SceneManager* sceneManager = ogreGetSceneManager();
	Ogre::MeshPtr v2Mesh = Ogre::MeshManager::getSingleton().load(
	    name, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	Ogre::Item* item = sceneManager->createItem(
	    name, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, Ogre::SCENE_DYNAMIC);
	return item;
}

Ogre::SceneNode* ogreNodeFromItem(Ogre::Item* meshHandle)
{
	Ogre::SceneNode* newSceneNode = nullptr;
	Ogre::SceneManager* sceneManager = ogreGetSceneManager();
	Ogre::SceneNode* rootSceneNode = sceneManager->getRootSceneNode();
	if (rootSceneNode)
	{
		Ogre::SceneNode* sceneNode = ((Ogre::SceneNode*)rootSceneNode->createChild());
		if (sceneNode)
		{
			sceneNode->attachObject(meshHandle);
			newSceneNode = sceneNode;
		}
	}
	return newSceneNode;
}

// An example only: Create a directional light. Feel free to tear this function apart to make more
// programmable lights
Ogre::SceneNode* ogreCreateLight()
{
	Ogre::SceneNode* newLightNode = nullptr;
	Ogre::SceneManager* sceneManager = ogreGetSceneManager();
	Ogre::SceneNode* rootSceneNode = sceneManager->getRootSceneNode();
	if (rootSceneNode)
	{
		Ogre::Light* light = sceneManager->createLight();
		Ogre::SceneNode* lightNode = ((Ogre::SceneNode*)rootSceneNode->createChild());
		newLightNode = lightNode;
		lightNode->attachObject(light);
		light->setPowerScale(1.0f);
		light->setType(Ogre::Light::LT_DIRECTIONAL);
		light->setDirection(Ogre::Vector3(-1, -1, -1).normalisedCopy());
		sceneManager->setAmbientLight((Ogre::ColourValue(0.3f, 0.5f, 0.7f) * 0.1f * 0.75f),
		                              (Ogre::ColourValue(0.6f, 0.45f, 0.3f) * 0.065f * 0.75f),
		                              ((-light->getDirection()) + (Ogre::Vector3::UNIT_Y * 0.2f)));
	}
	return newLightNode;
}

void ogreCreatePbsSpheres(Ogre::Root* root, Ogre::SceneManager* sceneManager)
{
	int numSpheres = 0;
	Ogre::HlmsManager* hlmsManager = root->getHlmsManager();

	assert(dynamic_cast<Ogre::HlmsPbs*>(hlmsManager->getHlms(Ogre::HLMS_PBS)));

	Ogre::HlmsPbs* hlmsPbs = static_cast<Ogre::HlmsPbs*>(hlmsManager->getHlms(Ogre::HLMS_PBS));

	const int numX = 8;
	const int numZ = 8;

	const float armsLength = 1.f;
	const float startX = (numX - 1) / 2.0f;
	const float startZ = (numZ - 1) / 2.0f;

	Ogre::TextureGpuManager* textureMgr = root->getRenderSystem()->getTextureGpuManager();
	Ogre::SceneNode* rootSceneNode = sceneManager->getRootSceneNode();

	// const Ogre::String meshNameV2 = "Sphere.mesh";
	const Ogre::String meshNameV2 = "Suzanne.mesh";
	{
		Ogre::MeshPtr v2Mesh = Ogre::MeshManager::getSingleton().load(
		    meshNameV2, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
	}

	for (int x = 0; x < numX; ++x)
	{
		for (int z = 0; z < numZ; ++z)
		{
			Ogre::String datablockName = "Test" + Ogre::StringConverter::toString(numSpheres++);
			Ogre::HlmsPbsDatablock* datablock = static_cast<Ogre::HlmsPbsDatablock*>(
			    hlmsPbs->createDatablock(datablockName, datablockName, Ogre::HlmsMacroblock(),
			                             Ogre::HlmsBlendblock(), Ogre::HlmsParamVec()));

			// Reflection texture. Uncomment once you've copied SaintPetersBasilica.dds to Materials/Textures
			// {
			// 	Ogre::TextureGpu* texture = textureMgr->createOrRetrieveTexture(
			// 	    "SaintPetersBasilica.dds", Ogre::GpuPageOutStrategy::Discard,
			// 	    Ogre::TextureFlags::PrefersLoadingFromFileAsSRGB, Ogre::TextureTypes::TypeCube,
			// 	    Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
			// 	    Ogre::TextureFilter::TypeGenerateDefaultMipmaps);

			// 	datablock->setTexture(Ogre::PBSM_REFLECTION, texture);
			// }

			datablock->setDiffuse(Ogre::Vector3(0.0f, 1.0f, 0.0f));

			datablock->setRoughness(std::max(0.02f, x / std::max(1.0f, (float)(numX - 1))));
			datablock->setFresnel(Ogre::Vector3(z / std::max(1.0f, (float)(numZ - 1))), false);

			Ogre::Item* item = nullptr;
			{
				item = sceneManager->createItem(
				    meshNameV2, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
				    Ogre::SCENE_DYNAMIC);
			}

			item->setDatablock(datablock);
			item->setVisibilityFlags(0x000000002);

			Ogre::SceneNode* sceneNode = ((Ogre::SceneNode*)rootSceneNode->createChild());
			if (!sceneNode)
				return;
			sceneNode->setPosition(
			    Ogre::Vector3(armsLength * x - startX, -3.0f, armsLength * z - startZ));
			sceneNode->attachObject(item);
		}
	}
}

//
// main!
//

int main()
{
	ogreInitialize();

	Ogre::Item* monkeyMesh = ogreLoadMesh("Suzanne.mesh");
	Ogre::SceneNode* monkeyNode = ogreNodeFromItem(monkeyMesh);
	Ogre::SceneNode* lightNode = ogreCreateLight();
	ogreCreatePbsSpheres(gOgreRoot, gSceneManager);

	float x = 0.f;
	float y = 0.f;
	float z = 0.f;

	while (true)
	{
		Ogre::WindowEventUtilities::messagePump();
		if (g_ogre_window_should_quit)
			break;

		monkeyNode->setPosition(x, y, z);
		x = (x + 0.01f);
		if (!(ogreRenderFrame()))
		{
			// Something bad happened
			break;
		}
	}

	ogreShutdown();
	return 0;
}
