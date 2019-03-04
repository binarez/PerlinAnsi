#include <iostream>
#include <string>
#include <algorithm>
#include <thread>
#include <chrono>
#include <future>

#include "stb_perlin.h"

//-------
// Il n'existe pas de fa�on standard d'interroger et de configurer le terminal.
// Ce programme ne fonctionne que sous Windows.
#include <Windows.h>
//-------

using namespace std;
const string SYMBOLES_GRIS{ " .;-+oO0#$&%" };

char generer(size_t x, size_t y, size_t tailleX, size_t tailleY)
{
	const float ECHELLE_X = 8.f;
	const float ECHELLE_Y = 4.f;
	const float unSurTailleX{ 1.f / tailleX };
	const float unSurTailleY{ 1.f / tailleY };

	// La fonction de perlin permet de produire du bruit coh�rent.
	// Un bruit coh�rent est une fonction qui retourne des nombres al�atoires avec un contr�le sur la variation.
	// La version que nous utilisons ici permet de g�n�rer du bruit 3D, mais nous l'utilisons en version 2D
	// lorsque nous sp�cifions z = 0 (3D -> 2D).
	//
	// Perlin retourne une valeur entre [-1.0, 1.0] pour les floats x, y et z en entr�e.
	// Le wrap (l'enroulement) permet de garder le signal entre certaines valeurs,
	// lors que nous sp�cifions 0, nous laissons le contr�le � Perlin et recevont dans l'intervale complet [-1.f, 1.f]
	// [-1.f, 1.f] --> +1.f --> [0.f, 2.f ] --> *0.5f --> [0.f, 1.f] --> *255 + 0.5 -> [0.5, 255.5] -> transtypage(int - laisser tomber les d�cimales) -> [0, 255]
	int _0_255 = static_cast< int >(((stb_perlin_noise3(x * unSurTailleX * ECHELLE_X,
														y * unSurTailleY * ECHELLE_Y,
														/*z*/ 0.f,
														0, 0, 0)	// wrap / enroulement
										+ 1.f) * 0.5f * 255) + 0.5f);
	_0_255 = min(max(0, _0_255), 255);	// Restreindre � [0, 255]
	const char carac = SYMBOLES_GRIS.at(static_cast< int >(_0_255 / 255.f * SYMBOLES_GRIS.length() + 0.5f));
	return carac;
}

void ecrireLigne(size_t y, size_t tailleX, size_t tailleY)
{
	string line( tailleX, ' ' );
	for (size_t x = 0; x < tailleX; x++)
	{
		line[x] = generer(x, y, tailleX, tailleY);
	}
	cout << line << endl;
	cout.flush();
}

void obtenirTailleFenetre(size_t & largeur, size_t & hauteur)
{
	const COORD tailleFenetre{ GetLargestConsoleWindowSize(GetStdHandle(STD_OUTPUT_HANDLE)) };
	largeur = tailleFenetre.X;
	hauteur = tailleFenetre.Y;
}

bool quitter(void)
{
	// Tester toutes les touches du clavier
	for (int k{ 8 }; k <= 254; ++k)
	{
		if (GetKeyState(k) & 0x8000)	// Touche appuy�e?
		{
			return true;
		}
	}
	return false;
}

int main(void)
{
	setlocale(LC_ALL, "");

	size_t tailleX{ 80 };
	size_t tailleY{ 25 };

	// Configurer la console
	::SendMessage(::GetConsoleWindow(), WM_SYSKEYDOWN, VK_RETURN, 0x20000000);
	obtenirTailleFenetre(tailleX, tailleY);
	system(("mode " + to_string(tailleX) + ',' + to_string(tailleY)).c_str());	// Taille MS-DOS
	system("cls");	// Effacer MS-DOS

	const auto attente{ chrono::milliseconds(40) };
	size_t y{ 0 };
	bool fin{ false };
	while (!fin)
	{
		obtenirTailleFenetre(tailleX, tailleY);
		y++;
		ecrireLigne(y, tailleX, tailleY);

		std::future< bool > future{ std::async(std::launch::async, quitter) };
		if (future.wait_for(attente) == std::future_status::ready)
		{
			fin = future.get();
		}
	}

	return 0;
}
