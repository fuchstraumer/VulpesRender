// VulpesRender2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "scenes\NBody.h"

int main(){

	using namespace vulpes;
	nbody::NBodyScene scene;
	scene.RenderLoop();
	return 0;
}


