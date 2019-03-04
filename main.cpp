#include <iostream>
#include <string>
#include <algorithm>
#include <thread>
#include <chrono>
#include <future>
#include <limits>

#include "stb_perlin.h"

//-------
// Il n'existe pas de façon standard d'interroger et de configurer le terminal.
// Ce programme ne fonctionne que sous Windows.
#include <Windows.h>
//-------

using namespace std;
//const string SYMBOLES_GRIS{ " .;-+oO0#$&%" };
const string SYMBOLES_GRIS{ " .;o0#&%" };
//const string SYMBOLES_GRIS{ ".:*IVFNM" };

enum class Operation
{
	Rien,
	PlusGrand,
	PlusPetit,
	Quitter,
};

char generer(size_t x, size_t y, size_t tailleX, size_t tailleY, float echelle)
{
	echelle = (echelle / 4);	// Ajuster l'échelle
	const float ECHELLE_X = 8.f;
	const float ECHELLE_Y = 4.f;
	const float unSurTailleX{ 1.f / tailleX };
	const float unSurTailleY{ 1.f / tailleY };

	// La fonction de perlin permet de produire du bruit cohérent.
	// Un bruit cohérent est une fonction qui retourne des nombres aléatoires avec un contrôle sur la variation.
	// La version que nous utilisons ici permet de générer du bruit 3D, mais nous l'utilisons en version 2D
	// lorsque nous spécifions z = 0 (3D -> 2D).
	//
	// Perlin retourne une valeur entre [-1.0, 1.0] pour les floats x, y et z en entrée.
	// Le wrap (l'enroulement) permet de garder le signal entre certaines valeurs,
	// lors que nous spécifions 0, nous laissons le contrôle à Perlin et recevont dans l'intervale complet [-1.f, 1.f]
	// [-1.f, 1.f] --> +1.f --> [0.f, 2.f ] --> *0.5f --> [0.f, 1.f] --> *255 + 0.5 -> [0.5, 255.5] -> transtypage(int - laisser tomber les décimales) -> [0, 255]
	int _0_255 = static_cast< int >(((stb_perlin_noise3(x * unSurTailleX * ECHELLE_X * echelle,
														y * unSurTailleY * ECHELLE_Y * echelle,
														/*z*/ 0.f,
														0, 0, 0)	// wrap / enroulement
										+ 1.f) * 0.5f * 255) + 0.5f);
	_0_255 = min(max(0, _0_255), 255);	// Restreindre à [0, 255]
	const char carac = SYMBOLES_GRIS.at(static_cast< int >(	_0_255 / 256.f * SYMBOLES_GRIS.length()));
	return carac;
}

void ecrireLigne(size_t y, size_t tailleX, size_t tailleY, float echelle)
{
	string line( tailleX, ' ' );
	for (size_t x = 0; x < tailleX; x++)
	{
		line[x] = generer(x, y, tailleX, tailleY, echelle);
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

Operation interagir(void)
{
	static auto derniereLecture{ std::chrono::system_clock::now() };
	static const auto attenteLecture = chrono::milliseconds(500);
	const auto maintenant{ std::chrono::system_clock::now() };
	const bool okPeutLire = ((derniereLecture + attenteLecture) < maintenant);
	while (okPeutLire)
	{
		if (GetAsyncKeyState(VK_ADD) & 0x8000)
		{
			derniereLecture = maintenant;
			return Operation::PlusGrand;
		}
		else if (GetAsyncKeyState(VK_SUBTRACT) & 0x8000)
		{
			derniereLecture = maintenant;
			return Operation::PlusPetit;
		}
		else if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
		{
			derniereLecture = maintenant;
			return Operation::Quitter;
		}
	}
	return Operation::Rien;
}

void configurerConsole(size_t & largeur, size_t & hauteur)
{
	::SendMessage(::GetConsoleWindow(), WM_SYSKEYDOWN, VK_RETURN, 0x20000000); // Plein écran
	obtenirTailleFenetre(largeur, hauteur);
	system(("mode " + to_string(largeur) + ',' + to_string(hauteur)).c_str());	// Taille MS-DOS
	cout << endl;
}

int main(void)
{
	//setlocale(LC_ALL, "C");

	size_t tailleX{ 80 };
	size_t tailleY{ 25 };
	configurerConsole(tailleX, tailleY);

	std::future< Operation > future{ std::async(std::launch::async, interagir) };
	const string separateur(tailleX, '-');
	const auto attente{ chrono::milliseconds(15) };
	size_t y{ 0 };
	float echelle{ 4 };
	bool fin{ false };
	while (!fin)
	{
		obtenirTailleFenetre(tailleX, tailleY);
		y++;
		ecrireLigne(y, tailleX, tailleY, echelle);

		this_thread::sleep_for(attente);
		if (future.wait_for(chrono::milliseconds::zero()) == std::future_status::ready)
		{
			const Operation op{ future.get() };
			future = std::async(std::launch::async, interagir);
			switch (op)
			{
			case Operation::PlusPetit:
				echelle = echelle / 1.2f;
				cout << separateur << endl;
				break;
			case Operation::PlusGrand:
				echelle = echelle * 1.2f;
				cout << separateur << endl;
				break;
			case Operation::Quitter:
				fin = true;
				break;
			}
		}
	}

	return 0;
}
