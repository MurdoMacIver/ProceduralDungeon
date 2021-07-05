#include "pch.h"
#include "Input.h"


Input::Input()
{
}

Input::~Input()
{
}

void Input::Initialise(HWND window)
{
	m_keyboard = std::make_unique<DirectX::Keyboard>();
	m_mouse = std::make_unique<DirectX::Mouse>();
	m_mouse->SetWindow(window);
	m_quitApp = false;

	m_GameInput.forward		= false;
	m_GameInput.back		= false;
	m_GameInput.right		= false;
	m_GameInput.left		= false;
	m_GameInput.rotRight	= false;
	m_GameInput.rotLeft		= false;
	m_GameInput.lookUp		= false;
	m_GameInput.lookDown	= false;
	m_GameInput.mapPlayer		= false;

	m_GameInput.perlin		= false;
}

void Input::Update()
{
	auto kb = m_keyboard->GetState();	//updates the basic keyboard state
	m_KeyboardTracker.Update(kb);		//updates the more feature filled state. Press / release etc. 
	auto mouse = m_mouse->GetState();   //updates the basic mouse state
	m_MouseTracker.Update(mouse);		//updates the more advanced mouse state. 

	if (kb.Escape)// check has escape been pressed.  if so, quit out. 
	{
		m_quitApp = true;
	}

	//A key
	if (kb.A)	m_GameInput.left = true;
	else		m_GameInput.left = false;
	
	//D key
	if (kb.D)	m_GameInput.right = true;
	else		m_GameInput.right = false;

	//W key
	if (kb.W)	m_GameInput.forward	 = true;
	else		m_GameInput.forward = false;

	//S key
	if (kb.S)	m_GameInput.back = true;
	else		m_GameInput.back = false;

	// space generates a midpoint terrain
	if (kb.Space) m_GameInput.generate = true;
	else		m_GameInput.generate = false;
	// P generates a perlin noise terrain
	if (kb.P) m_GameInput.perlin = true;
	else		m_GameInput.perlin = false;
	// M generates a midpoint terrain again, but this time there no peaks and sharp edges
	if (kb.M) m_GameInput.midpoint = true;
	else	  m_GameInput.midpoint = false;
	// F smooths the terrain and textures

	// L creates the maze or labryith 
	if (m_KeyboardTracker.IsKeyPressed(DirectX::Keyboard::L)) m_GameInput.automata = true;
	else      m_GameInput.automata = false;

}

bool Input::Quit()
{
	return m_quitApp;
}

InputCommands Input::getGameInput()
{
	return m_GameInput;
}
