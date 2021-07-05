//
// Game.cpp
//

#include "pch.h"
#include "Game.h"


//toreorganise
#include <fstream>

/*set the maximum number lives before player loses*/
#define MAX_LIVES 10

extern void ExitGame();

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace ImGui;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
	m_deviceResources->RegisterDeviceNotify(this);
}

Game::~Game()
{
#ifdef DXTK_AUDIO
    if (m_audEngine)
    {
        m_audEngine->Suspend();
    }
#endif
}

#pragma region Initialize
// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{

	m_input.Initialise(window);

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

	//setup imgui.  its up here cos we need the window handle too
	//pulled from imgui directx11 example
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(window);		//tie to our window // hook it to window
	ImGui_ImplDX11_Init(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext());	//tie to directx

	m_fullscreenRect.left = 20;
	m_fullscreenRect.top = 20;
	m_fullscreenRect.right = 500;
	m_fullscreenRect.bottom = 300;

	m_CameraViewRect.left = 500;
	m_CameraViewRect.top = 0;
	m_CameraViewRect.right = 800;
	m_CameraViewRect.bottom = 240;

	//setup light
	m_Light.setAmbientColour(0.3f, 0.3f, 0.3f, 1.0f);
	m_Light.setDiffuseColour(1.0f, 1.0f, 1.0f, 1.0f);
	m_Light.setPosition(12.0f, 1.0f, 12.0f);
	m_Light.setDirection(-1.0f, -1.0f, 0.0f);

	// Variable Initialisation
	isIntersecting = false;
	m_playerInGrid = false;
	m_isPlaying = false;
	mapPlayer = false;
	enableMagic = false;
	m_score = 0;
	m_playerCells = 0;
	
	// Place the Player on the grid

	SetPlayerOnGrid();

	
#pragma region AudioSetUp
#ifdef DXTK_AUDIO
    // Create DirectXTK for Audio objects
    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
    eflags = eflags | AudioEngine_Debug;
#endif

    m_audEngine = std::make_unique<AudioEngine>(eflags);

    m_audioEvent = 0;
    m_audioTimerAcc = 10.f;
    m_retryDefault = false;

    m_waveBank = std::make_unique<WaveBank>(m_audEngine.get(), L"adpcmdroid.xwb");

    m_soundEffect = std::make_unique<SoundEffect>(m_audEngine.get(), L"MusicMono_adpcm.wav");
    m_effect1 = m_soundEffect->CreateInstance();
    m_effect2 = m_waveBank->CreateInstance(10);

    m_effect1->Play(true);
    m_effect2->Play();
#endif
#pragma endregion

}
#pragma endregion

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
	//take in input
	m_input.Update();								//update the hardware
	m_gameInputCommands = m_input.getGameInput();	//retrieve the input for our game
	
	//Update all game objects
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

	//Render all game content. 
    Render();
#pragma region AudioFrameUpdate
#ifdef DXTK_AUDIO
    // Only update audio engine once per frame
    if (!m_audEngine->IsCriticalError() && m_audEngine->Update())
    {
        // Setup a retry in 1 second
        m_audioTimerAcc = 1.f;
        m_retryDefault = true;
    }
#endif
#pragma endregion
	
}
#pragma endregion

// Updates the world.
#pragma region UpdateWorld
void Game::Update(DX::StepTimer const& timer)
{	

	float deltaTime = float(timer.GetElapsedSeconds());
	
	auto device = m_deviceResources->GetD3DDevice();


	// The camera is the player
										/********************************************************************
										*																	*
										*				Rotation of the camera or player object				*
										*																	*
										*********************************************************************/
	if (m_gameInputCommands.left)
	{
		Vector3 rotation = m_Camera01.getRotation();
		rotation.y = rotation.y -= m_Camera01.getRotationSpeed();
		m_Camera01.setRotation(rotation);
	}
	if (m_gameInputCommands.right)
	{
		Vector3 rotation = m_Camera01.getRotation();
		rotation.y = rotation.y += m_Camera01.getRotationSpeed();
		m_Camera01.setRotation(rotation);
	}
										/********************************************************************
										*																	*
										*				Movement of the camera or player object				*
										*																	*
										*********************************************************************/
	if (!isIntersecting)
	{
		if (m_gameInputCommands.forward)
		{
			Vector3 position = m_Camera01.getPosition();
			position += (m_Camera01.getForward() * m_Camera01.getMoveSpeed());
			if (position.x >= 25.f) {
				position.x = 25.f;
			}
			if (position.x <= 0.f) {
				position.x = 0.f;
			}
			if (position.z >= 25.f)
				position.z = 25.0f;
			if (position.z <= 0.f)
				position.z = 0.f;
			m_Camera01.setPosition(position);
		}
		if (m_gameInputCommands.back)
		{
			Vector3 position = m_Camera01.getPosition();
			position -= (m_Camera01.getForward() * m_Camera01.getMoveSpeed());
			if (position.x >= 25.f) {
				position.x = 25.f;
			}
			if (position.x <= 0.f) {
				position.x = 0.f;
			}
			if (position.z >= 25.f)
				position.z = 25.0f;
			if (position.z <= 0.f)
				position.z = 0.f;
			m_Camera01.setPosition(position);
		}
		m_currentPlayerPosition = m_Camera01.getPosition();
	}
	else {
								/********************************************************************
								*																	*
								*				On player collision reset player back to start		*
								*																	*
								*********************************************************************/
		Vector3 newPosition = Vector3(m_colliderPosition.x + 0.5f, 0.5f, m_colliderPosition.z + 0.5f);
		m_Camera01.setPosition(m_playerStartPosition);
		m_Camera01.setRotation(m_playerStartRotation);
		if (m_score <= 0) 
		{
			m_score = 0;
		}
		else {
			m_score -= 10;
		}
		isIntersecting = false;
	}

	if (m_gameInputCommands.up)
	{
		Vector3 position = m_Camera01.getPosition(); 
		position.y = position.y += m_Camera01.getMoveSpeed();
		if (position.y >= 10.f) position.y = 10.0f;
		m_Camera01.setPosition(position);
	}
	if (m_gameInputCommands.down)
	{
		Vector3 position = m_Camera01.getPosition();
		position.y = position.y -= m_Camera01.getMoveSpeed(); 
		if (position.y <= 2.0f) position.y = 2.0f;
		m_Camera01.setPosition(position);
	}

	/* Removed for final game */
	if (m_gameInputCommands.lookUp)
	{
		Vector3 rotation = m_Camera01.getRotation();
		rotation.x = rotation.x += m_Camera01.getRotationSpeed();
		if (rotation.x >= -10.0f) rotation.x = -10.f;
		m_Camera01.setRotation(rotation);
	}
	if (m_gameInputCommands.lookDown)
	{
		Vector3 rotation = m_Camera01.getRotation();
		rotation.x = rotation.x -= m_Camera01.getRotationSpeed();
		if (rotation.x <= -170.f) rotation.x = -170.f;
		m_Camera01.setRotation(rotation);
	}

	if (m_gameInputCommands.generate)
	{
		m_Terrain.GenerateHeightMap(device);
	}

	if (m_gameInputCommands.midpoint) {
		m_Terrain.GenerateMidpointHeightMap(device);
	}

	if (m_gameInputCommands.perlin)
	{
		m_Terrain.GenerateHeightMap(device);
	}
	
	if (m_gameInputCommands.smoothen) {
		m_Terrain.SmoothenHeightMap(device);
	}
	
	if (m_gameInputCommands.automata) {
		m_Grid.nextGeneration();
	}
	

	m_Camera01.Update();
	
	m_Terrain.Update();		

	m_view = m_Camera01.getCameraMatrix();
	m_world = Matrix::Identity;

	/*create our UI*/ // Toggle on / off gui
	SetupGUI();
	
	std::wstring ws2 = L" Camera Position";
	std::wstring ws1 = std::to_wstring(m_playerCells);
	std::wstring s(ws1);
	s += std::wstring(ws2);
	const wchar_t* debug = s.c_str();
	debugLine = debug;
	
											/********************************************************************
											*																	*
											*					Collision with pickup							*
											*																	*
											*********************************************************************/

	if (Intersect(m_playerBox, m_goldcoinBox)) 
	{
		debugLine = L"gold coin collected";
		m_Grid.initializeGrid();
		SetPlayerOnGrid();
		m_score += 20;

	}
	else {
		debugLine = L"";
		isIntersecting = false;
	}

	// Collision check
	if (m_Grid.GetInitialised()) {
		for (int i = 0; i < m_Grid.Size(); i++)
		{
			for (int j = 0; j < m_Grid.Size(); j++)
			{
				if (IntersectCell(m_playerBox, m_Grid.cellMatrix[i][j]))
				{
					if (m_Grid.cellMatrix[i][j].GetState() == 1)
					{

						
						m_colliderPosition = m_Grid.cellMatrix[i][j].GetPosition(); // finds where the collisions happens on the grid
						
						if (m_colliderPosition == m_playerStartPosition)
						{
							m_Grid.initializeGrid();
							SetPlayerOnGrid();
							m_playerLives = MAX_LIVES;
						}
						else {
							debugLine = L"Hit Wall!";
							isIntersecting = true;
						}
					}
					else {

					}

				}
				else {
					m_isPlaying = true;
					m_playerLives = MAX_LIVES;

				}
			}
		}
		
	}

#pragma region Audio
#ifdef DXTK_AUDIO
    m_audioTimerAcc -= (float)timer.GetElapsedSeconds();
    if (m_audioTimerAcc < 0)
    {
        if (m_retryDefault)
        {
            m_retryDefault = false;
            if (m_audEngine->Reset())
            {
                // Restart looping audio
                m_effect1->Play(true);
            }
        }
        else
        {
            m_audioTimerAcc = 4.f;

            m_waveBank->Play(m_audioEvent++);

            if (m_audioEvent >= 11)
                m_audioEvent = 0;
        }
    }
#endif
#pragma endregion

  
	if (m_input.Quit())
	{
		ExitGame();
	}
}

#pragma endregion



#pragma region Frame Render
// Draws the scene.
void Game::Render()
{	
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");
    auto context = m_deviceResources->GetD3DDeviceContext();
	auto renderTargetView = m_deviceResources->GetRenderTargetView();
	auto depthTargetView = m_deviceResources->GetDepthStencilView();

	// Drawing of the skybox

	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthNone(), 0);
	context->RSSetState(m_states->CullNone());

	m_world = SimpleMath::Matrix::Identity;
	SimpleMath::Matrix skyboxPosition = SimpleMath::Matrix::CreateTranslation(m_Camera01.getPosition());


	m_world = m_world * skyboxPosition;
	// Turn our shaders on and the set parameters 
												/*in the form*/

												/****************************************************
												*			 struct InputType						*
												*				{									*
												*					float4 position : SV_POSITION;	*
												*					float2 tex : TEXCOORD0;			*
												*					float3 normal : NORMAL;			*
												*					float3 position3D : TEXCOORD2;	*
												*				};									*
												****************************************************/
												/*with respect to the parameters of the world, view, projection, light and texture*/
	m_BasicShaderPair_skydome.EnableShader(context);
	m_BasicShaderPair_skydome.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture5.Get());
	//render the model
	m_Skybox.Render(context);

	//Reset Rendering states. 
	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
	context->RSSetState(m_states->CullClockwise());
	//context->RSSetState(m_states->Wireframe()); // this enables wireframe

	// mapPlayer Render to Texture
	RenderTexturemapPlayer();

	if (enableMagic)
	{
		Magic();
	}
	else {
		//prepare transform for floor object. 
		m_world = SimpleMath::Matrix::Identity; //set world back to identity
		SimpleMath::Matrix newPosition3 = SimpleMath::Matrix::CreateTranslation(0.0f, 0.0f, 0.0f);
		SimpleMath::Matrix newScale = SimpleMath::Matrix::CreateScale(0.1);		
		m_world = m_world * newScale * newPosition3;

		//setup and draw floor
		m_BasicShaderPair_Heightmap.EnableShader(context);
		m_BasicShaderPair_Heightmap.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture1.Get());
		m_Terrain.Render(context);

		// CellGrid

		for (int i = 0; i < m_Grid.Size(); i++)
		{
			for (int j = 0; j < m_Grid.Size(); j++)
			{

				Vector3 tempPosition = DirectX::SimpleMath::Vector3(i, 0.5f, j);
				m_Grid.cellMatrix[i][j].SetPosition(tempPosition);
				m_Grid.cellMatrix[i][j].SetCentre(tempPosition);

				m_world = SimpleMath::Matrix::Identity; 
				newPosition3 = SimpleMath::Matrix::CreateTranslation(m_Grid.cellMatrix[i][j].GetPosition());
				m_world = m_world * newPosition3;

				if (m_Grid.cellMatrix[i][j].GetState() == 1)
				{
					m_BasicShaderPair_skydome.EnableShader(context);
					m_BasicShaderPair_skydome.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture2.Get());
					m_cellBox.SetCentre(m_Grid.cellMatrix[i][j].GetPosition());
					m_cellBox.Render(context);
				}
				else if (m_Grid.cellMatrix[i][j].GetState() == 3)
				{
					//setup and draw goldcoin
					m_BasicShaderPair_skydome.EnableShader(context);
					m_BasicShaderPair_skydome.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture3.Get());
					m_goldcoinBox.SetCentre(m_Grid.cellMatrix[i][j].GetPosition());
					m_goldcoinBox.Render(context);
				}
				else {
					
				}
			}
		}

																/*set-up the player transformation*/
		m_world = SimpleMath::Matrix::Identity;
		newPosition3 = SimpleMath::Matrix::CreateTranslation(m_Camera01.getPosition().x, m_Camera01.getPosition().y, m_Camera01.getPosition().z);
		m_playerBox.SetCentre(m_Camera01.getPosition());
		newScale = SimpleMath::Matrix::CreateScale(1);
		m_world = m_world * newScale * newPosition3;

		//setup and draw rectangle
		m_BasicShaderPair_skydome.EnableShader(context);
		m_BasicShaderPair_skydome.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, NULL); 
		m_playerBox.Render(context);
	}
	

	if (!m_Grid.GetInitialised()) {
		m_Grid.SetInitialised(true);
	}

	//render our GUI
	//ImGui::Render();
	//ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	// Hides GUI
	ImGui::EndFrame();

	/* FPS */
	std::wstring ws2 = L" FPS";
	std::wstring ws1 = std::to_wstring(m_timer.GetFramesPerSecond());
	std::wstring s_fps(ws1);
	s_fps += std::wstring(ws2);
	const wchar_t* fps = s_fps.c_str();

	/* PlayerScore */
	std::wstring ws4 = L" points";
	std::wstring ws3 = std::to_wstring(m_score);
	std::wstring s_playerScore(ws3);
	s_playerScore += std::wstring(ws4);
	const wchar_t* playerScore = s_playerScore.c_str();

	/* Distance to goldcoin */
	std::wstring ws5 = std::to_wstring(*m_Grid.GetDistance());
	std::wstring ws6 = L" cells";
	std::wstring s_goldcoinDistance(ws5);
	s_goldcoinDistance += std::wstring(ws6);
	const wchar_t* distance = s_goldcoinDistance.c_str();

	// Draw Text to the screen
	m_sprites->Begin();
	m_font->DrawString(m_sprites.get(), L"Find the gold coin to collect points", XMFLOAT2((m_deviceResources->GetScreenViewport().Width / 2) - 100.0f, m_deviceResources->GetScreenViewport().TopLeftY), Colors::ForestGreen);
	if (*m_Grid.GetDistance() != 0)
	{

	}
	else 
	{

	}
	m_font->DrawString(m_sprites.get(), L"Press L to Generate a new Maze", XMFLOAT2((m_deviceResources->GetScreenViewport().Width / 2) - 400.0f, m_deviceResources->GetScreenViewport().Height - 40.f), Colors::ForestGreen);
	m_font->DrawString(m_sprites.get(), playerScore, XMFLOAT2(m_deviceResources->GetScreenViewport().Width - 110.0f, m_deviceResources->GetScreenViewport().Height - 40.f), Colors::White);
	m_sprites->End();

	// Draw our sprite with the render texture displayed on it.
	if (mapPlayer)
	{
		m_sprites->Begin();
		m_sprites->Draw(m_FirstRenderPass->getShaderResourceView(), m_fullscreenRect);
		m_sprites->End();
	}

    // Show the new frame.
    m_deviceResources->Present();
}

void Game::RenderTexturemapPlayer() {

	auto context = m_deviceResources->GetD3DDeviceContext();
	auto renderTargetView = m_deviceResources->GetRenderTargetView();
	auto depthTargetView = m_deviceResources->GetDepthStencilView();
	// Set the render target to be the render to texture.
	m_FirstRenderPass->setRenderTarget(context);
	// Clear the render to texture.
	m_FirstRenderPass->clearRenderTarget(context, 0.8f, 0.8f, 0.8f, 1.0f);

	//prepare transform for floor object. 
	m_world = SimpleMath::Matrix::Identity; 
	SimpleMath::Matrix newPosition3 = SimpleMath::Matrix::CreateTranslation(0.0f, 0.0f, 0.0f);
	SimpleMath::Matrix newScale = SimpleMath::Matrix::CreateScale(0.1);		
	m_world = m_world * newScale * newPosition3;

	//setup and draw floor
	m_BasicShaderPair_Heightmap.EnableShader(context);
	m_BasicShaderPair_Heightmap.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture1.Get());
	m_Terrain.Render(context);

	// CellGrid

	for (int i = 0; i < m_Grid.Size(); i++)
	{
		for (int j = 0; j < m_Grid.Size(); j++)
		{

			Vector3 tempPosition = DirectX::SimpleMath::Vector3(i, 0.5f, j);
			m_Grid.cellMatrix[i][j].SetPosition(tempPosition);
			m_Grid.cellMatrix[i][j].SetCentre(tempPosition);

			m_world = SimpleMath::Matrix::Identity; 
			newPosition3 = SimpleMath::Matrix::CreateTranslation(m_Grid.cellMatrix[i][j].GetPosition());
			m_world = m_world * newPosition3;

			if (m_Grid.cellMatrix[i][j].GetState() == 1)
			{
				m_BasicShaderPair_skydome.EnableShader(context);
				m_BasicShaderPair_skydome.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture2.Get());
				m_cellBox.SetCentre(m_Grid.cellMatrix[i][j].GetPosition());
				m_cellBox.Render(context);
			}
			else if (m_Grid.cellMatrix[i][j].GetState() == 3)
			{
				//setup and draw goldcoin
				m_BasicShaderPair_skydome.EnableShader(context);
				m_BasicShaderPair_skydome.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture3.Get());
				m_goldcoinBox.SetCentre(m_Grid.cellMatrix[i][j].GetPosition());
				m_goldcoinBox.Render(context);
			}
			else {
				
			}
		}
	}


	m_world = SimpleMath::Matrix::Identity;
	newPosition3 = SimpleMath::Matrix::CreateTranslation(m_Camera01.getPosition().x, m_Camera01.getPosition().y, m_Camera01.getPosition().z);
	m_playerBox.SetCentre(m_Camera01.getPosition());
	newScale = SimpleMath::Matrix::CreateScale(1);
	m_world = m_world * newScale * newPosition3;

	//setup and draw rectangle
	m_BasicShaderPair_skydome.EnableShader(context);
	m_BasicShaderPair_skydome.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture4.Get());
	m_playerBox.Render(context);
	
	context->OMSetRenderTargets(1, &renderTargetView, depthTargetView);
}

#pragma region collsion detection
									/************************************************************************************************************
									*																											*
									*																											*
									*	collsion detection done by setting up two boxes and then check collisions between them using AABB		*
									*																											*
									*																											*
									*																											*	
									*************************************************************************************************************/
bool Game::Intersect(ModelClass a, ModelClass b) 
{
	float aMinX, aMaxX, aMinY, aMaxY, aMinZ, aMaxZ = 0.f;
	float bMinX, bMaxX, bMinY, bMaxY, bMinZ, bMaxZ = 0.f;

	float widthA = a.GetDimensions().x;
	float widthB = b.GetDimensions().x;
	float heightA = a.GetDimensions().y;
	float heightB = b.GetDimensions().y;
	float depthA = a.GetDimensions().z;
	float depthB = b.GetDimensions().z;

	aMinX = a.GetCentre().x - (widthA / 2);
	aMaxX = a.GetCentre().x + (widthA / 2);

	aMinY = a.GetCentre().y - (heightA / 2);
	aMaxY = a.GetCentre().y + (heightA / 2);

	aMinZ = a.GetCentre().z - (depthA / 2);
	aMaxZ = a.GetCentre().z + (depthA / 2);

	bMinX = b.GetCentre().x - (widthB / 2);
	bMaxX = b.GetCentre().x + (widthB / 2);

	bMinY = b.GetCentre().y - (heightB / 2);
	bMaxY = b.GetCentre().y + (heightB / 2);

	bMinZ = b.GetCentre().z - (depthB / 2);
	bMaxZ = b.GetCentre().z + (depthB / 2);

	return (aMinX <= bMaxX && aMaxX >= bMinX) &&
		   (aMinY <= bMaxY && aMaxY >= bMinY) &&
		   (aMinZ <= bMaxZ && aMaxZ >= bMinZ);
}

bool Game::IntersectCell(ModelClass a, Cell b)
{
	float aMinX, aMaxX, aMinY, aMaxY, aMinZ, aMaxZ = 0.f;
	float bMinX, bMaxX, bMinY, bMaxY, bMinZ, bMaxZ = 0.f;

	float widthA = a.GetDimensions().x;
	float widthB = b.GetDimensions().x;
	float heightA = a.GetDimensions().y;
	float heightB = b.GetDimensions().y;
	float depthA = a.GetDimensions().z;
	float depthB = b.GetDimensions().z;

	aMinX = a.GetCentre().x - (widthA / 2);
	aMaxX = a.GetCentre().x + (widthA / 2);

	aMinY = a.GetCentre().y - (heightA / 2);
	aMaxY = a.GetCentre().y + (heightA / 2);

	aMinZ = a.GetCentre().z - (depthA / 2);
	aMaxZ = a.GetCentre().z + (depthA / 2);

	bMinX = b.GetCentre().x - (widthB / 2);
	bMaxX = b.GetCentre().x + (widthB / 2);

	bMinY = b.GetCentre().y - (heightB / 2);
	bMaxY = b.GetCentre().y + (heightB / 2);

	bMinZ = b.GetCentre().z - (depthB / 2);
	bMaxZ = b.GetCentre().z + (depthB / 2);

	if (aMinX <= bMaxX && aMaxX >= bMinX) 
	{
		m_backwardCollision = true;
	}
	else if (aMinZ <= bMaxZ && aMaxZ >= bMinZ)
	{
		m_forwardCollision = true;
	}

	return (aMinX <= bMaxX && aMaxX >= bMinX) &&
		(aMinY <= bMaxY && aMaxY >= bMinY) &&
		(aMinZ <= bMaxZ && aMaxZ >= bMinZ);
}
#pragma endregion

#pragma region place player on grid
void Game::SetPlayerOnGrid() 
{
	for (int i = 0; i < m_Grid.Size(); i++)
	{
		for (int j = 0; j < m_Grid.Size(); j++)
		{
			int randomValue = rand() % (m_Grid.Size() - 1) + 1;

			m_playerStartPosition = Vector3(randomValue, 0.5f, randomValue);
			m_playerStartRotation = Vector3(-90.0f, 0.0f, 0.0f);
			if (m_Grid.cellMatrix[i][j].GetState() == 2)
			{
				m_Grid.cellMatrix[i][j].SetPosition(m_playerStartPosition);
				m_Grid.cellMatrix[i][j].SetCentre(m_playerStartPosition);

				m_Grid.ResetPlayerInStateMatrix(j, i);

				m_Camera01.setPosition(m_Grid.cellMatrix[i][j].GetPosition());
				m_Camera01.setRotation(m_playerStartRotation);
				m_playerCells++;
				return;

			}
		}
	}
}
#pragma endregion

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
}

#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
}

void Game::OnDeactivated()
{
}

void Game::OnSuspending()
{
#ifdef DXTK_AUDIO
    m_audEngine->Suspend();
#endif
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

#ifdef DXTK_AUDIO
    m_audEngine->Resume();
#endif
}

void Game::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();
}

#ifdef DXTK_AUDIO
void Game::NewAudioDevice()
{
    if (m_audEngine && !m_audEngine->IsAudioDevicePresent())
    {
        // Setup a retry in 1 second
        m_audioTimerAcc = 1.f;
        m_retryDefault = true;
    }
}
#endif

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    width = 1920;
    height = 1080;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto device = m_deviceResources->GetD3DDevice();

    m_states = std::make_unique<CommonStates>(device);
    m_fxFactory = std::make_unique<EffectFactory>(device);
    m_sprites = std::make_unique<SpriteBatch>(context);
    m_font = std::make_unique<SpriteFont>(device, L"SegoeUI_18.spritefont");
	m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(context);
	m_postProcess = std::make_unique<BasicPostProcess>(device);

	//setup our terrain // first step for generation terrain, flat surface xz plane through 2D Array
	//m_Terrain.Initialize(device, 128, 128);
	//m_Terrain.Initialize(device, 129, 129);
	m_Terrain.Initialize(device, 257, 257); // 257

	m_Skybox.InitializeBox(device, 1.0f, 1.0f, 1.0f);

	m_cellBox.InitializeBox(device, 1, 1, 1);
	m_playerBox.InitializeBox(device, 0.5, 0.5, 0.5); // more accurate collision detection
	m_goldcoinBox.InitializeBox(device, 1, 1, 1);

	m_Grid.initializeGrid();

	//load and set up our Vertex and Pixel Shaders
	m_BasicShaderPair_Heightmap.InitStandard(device, L"light_vs.cso", L"light_ps.cso");
	m_BasicShaderPair_skydome.InitStandard(device, L"light_vs.cso", L"light_ps_skydome.cso");

	//load Textures
	CreateDDSTextureFromFile(device, L"blackobsidian.dds", nullptr, m_texture1.ReleaseAndGetAddressOf()); // terrain and floor
	CreateDDSTextureFromFile(device, L"brick.dds", nullptr, m_texture2.ReleaseAndGetAddressOf()); // walls of the maze

	//CreateDDSTextureFromFile(device, L"grass.dds", nullptr, m_texture1.ReleaseAndGetAddressOf()); // terrain and floor
	//CreateDDSTextureFromFile(device, L"hedge.dds", nullptr, m_texture2.ReleaseAndGetAddressOf()); // walls of the maze

	CreateDDSTextureFromFile(device, L"goldcoin.dds", nullptr, m_texture3.ReleaseAndGetAddressOf()); // goldcoin
	CreateDDSTextureFromFile(device, L"player.dds", nullptr, m_texture4.ReleaseAndGetAddressOf()); // player on mapPlayer
	CreateDDSTextureFromFile(device, L"starmap.dds", nullptr, m_texture5.ReleaseAndGetAddressOf()); // skybox
	//CreateDDSTextureFromFile(device, L"sky.dds", nullptr, m_texture5.ReleaseAndGetAddressOf()); // alternative skybox

	//Initialise Render to texture
	m_FirstRenderPass = new RenderTexture(device, 1920, 1080, 1, 2);	//last two properties are not used, cannot be zero or equal value //mapPlayer
	m_RenderTexture = new RenderTexture(device, 1920, 1080, 1, 2); //Magic
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto size = m_deviceResources->GetOutputSize();
    float aspectRatio = float(size.right) / float(size.bottom);
    float fovAngleY = 70.0f * XM_PI / 180.0f;

    // This is a simple example of change that can be made when the app is in
    // portrait or snapped view.
    if (aspectRatio < 1.0f)
    {
        fovAngleY *= 2.0f;
    }

    // This sample makes use of a right-handed coordinate system using row-major matrices.
    m_projection = Matrix::CreatePerspectiveFieldOfView(
        fovAngleY,
        aspectRatio,
        0.01f,
        100.0f
    );
}

void Game::SetupGUI()
{

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Sin Wave Parameters");
		ImGui::SliderFloat("Wave Amplitude",	m_Terrain.GetAmplitude(), 0.0f, 10.0f);
		ImGui::SliderFloat("Wavelength",		m_Terrain.GetWavelength(), 0.0f, 1.0f);
	ImGui::End();
}

void Game::Magic()
{
	auto context = m_deviceResources->GetD3DDeviceContext();
	auto renderTargetView = m_deviceResources->GetRenderTargetView();
	auto depthTargetView = m_deviceResources->GetDepthStencilView();
	// Set the render target to be the render to texture.
	m_RenderTexture->setRenderTarget(context);
	// Clear the render to texture.
	m_RenderTexture->clearRenderTarget(context, 0.0f, 0.0f, 0.0f, 1.0f);

	// Draw skybox here // translate to camera position

	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthNone(), 0);
	context->RSSetState(m_states->CullNone());

	m_world = SimpleMath::Matrix::Identity;
	SimpleMath::Matrix skyboxPosition = SimpleMath::Matrix::CreateTranslation(m_Camera01.getPosition());


	m_world = m_world * skyboxPosition;
	// Turn our shaders on,  set parameters // MuseumFloor
	m_BasicShaderPair_skydome.EnableShader(context);
	m_BasicShaderPair_skydome.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture5.Get());
	//render our model
	m_Skybox.Render(context);

	//Reset Rendering states. 
	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
	context->RSSetState(m_states->CullClockwise());

	//prepare transform for floor object. 
	m_world = SimpleMath::Matrix::Identity; //set world back to identity
	SimpleMath::Matrix newPosition3 = SimpleMath::Matrix::CreateTranslation(0.0f, 0.0f, 0.0f);
	SimpleMath::Matrix newScale = SimpleMath::Matrix::CreateScale(0.1);		//scale the terrain down a little. 
	m_world = m_world * newScale * newPosition3;

	//setup and draw floor
	m_BasicShaderPair_Heightmap.EnableShader(context);
	m_BasicShaderPair_Heightmap.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture1.Get());
	m_Terrain.Render(context);

	// CellGrid

	for (int i = 0; i < m_Grid.Size(); i++)
	{
		for (int j = 0; j < m_Grid.Size(); j++)
		{

			Vector3 tempPosition = DirectX::SimpleMath::Vector3(i, 0.5f, j);
			m_Grid.cellMatrix[i][j].SetPosition(tempPosition);
			m_Grid.cellMatrix[i][j].SetCentre(tempPosition);

			m_world = SimpleMath::Matrix::Identity; //set world back to identity
			newPosition3 = SimpleMath::Matrix::CreateTranslation(m_Grid.cellMatrix[i][j].GetPosition());
			m_world = m_world * newPosition3;

			if (m_Grid.cellMatrix[i][j].GetState() == 1)
			{
				m_BasicShaderPair_skydome.EnableShader(context);
				m_BasicShaderPair_skydome.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture2.Get());
				m_cellBox.SetCentre(m_Grid.cellMatrix[i][j].GetPosition());
				m_cellBox.Render(context);
			}
			else if (m_Grid.cellMatrix[i][j].GetState() == 3)
			{
				//setup and draw goldcoin // draw object
				m_BasicShaderPair_skydome.EnableShader(context);
				m_BasicShaderPair_skydome.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture3.Get());
				m_goldcoinBox.SetCentre(m_Grid.cellMatrix[i][j].GetPosition());
				m_goldcoinBox.Render(context);
			}
			else {
				// do nothing
			}
		}
	}

	/* First render player so that Intersect will know beforehand where player is in current point of time */

	//prepare transform for Player object. // set coordinates
	m_world = SimpleMath::Matrix::Identity; //set world back to identity
	newPosition3 = SimpleMath::Matrix::CreateTranslation(m_Camera01.getPosition().x, m_Camera01.getPosition().y, m_Camera01.getPosition().z);
	m_playerBox.SetCentre(m_Camera01.getPosition());
	newScale = SimpleMath::Matrix::CreateScale(1);
	m_world = m_world * newScale * newPosition3;

	//setup and draw rectangle
	m_BasicShaderPair_skydome.EnableShader(context);
	m_BasicShaderPair_skydome.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, NULL); // invisible collision box for player
	m_playerBox.Render(context);

	// Reset the render target back to the original back buffer and not the render to texture anymore.	
	context->OMSetRenderTargets(1, &renderTargetView, depthTargetView);

	// Apply post process effect
	m_postProcess->SetEffect(BasicPostProcess::GaussianBlur_5x5);
	m_postProcess->SetSourceTexture(m_RenderTexture->getShaderResourceView());
	m_postProcess->Process(context);
}

void Game::OnDeviceLost()
{
    m_states.reset();
    m_fxFactory.reset();
    m_sprites.reset();
    m_font.reset();
	m_batch.reset();
	m_testmodel.reset();
    m_batchInputLayout.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}
#pragma endregion
