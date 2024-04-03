#define WIN32_LEAN_AND_MEAN // To speed the build process: https://stackoverflow.com/questions/11040133/what-does-defining-win32-lean-and-mean-exclude-exactly

#include <iostream>
#include <windows.h>
#include <ws2tcpip.h>
#include <conio.h>
#include <string>
#include <ctime>
#include <iomanip>
#include <thread>
#include <chrono>
#include <sstream>
using namespace std;

#pragma comment (lib, "Ws2_32.lib")
#pragma warning (disable:4996)

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015" // a port is a logical construct that identifies a specific process or a type of network service - https://en.wikipedia.org/wiki/Port_(computer_networking)

#define PAUSE 1

// Accept a client socket
SOCKET ClientSocket = INVALID_SOCKET;

COORD server_smile;
COORD client_smile;

int server_coin_count = 0;
int client_coin_count = 0;

bool timeUp = false; // Флаг для визначення закінчення часу
char** map = nullptr;


enum class KeyCodes { LEFT = 75, RIGHT = 77, UP = 72, DOWN = 80, ENTER = 13, ESCAPE = 27, SPACE = 32, ARROWS = 224 };
enum class Colors { BLUE = 9, RED = 12, BLACK = 0, YELLOW = 14, DARKGREEN = 2 };

bool collision = false;

void UpdateGameTimeInWindowTitle() {
	auto start = chrono::system_clock::now(); // Запоминаем время начала игры

	while (!timeUp) { // Поки час не вичерпано
		auto now = chrono::system_clock::now(); // Отримуємо поточний час
		auto elapsed_minutes = chrono::duration_cast<chrono::minutes>(now - start); // Визначаємо пройшлий час з початку гри
		auto elapsed_seconds = chrono::duration_cast<chrono::seconds>(now - start); // Вычисляем прошедшее время с начала игры

		if (elapsed_minutes.count() >= 1) { // Якщо пройшла 1 хвилина
			auto now = chrono::system_clock::now(); // Получаем текущее время
			auto elapsed_seconds = chrono::duration_cast<chrono::seconds>(now - start); // Вычисляем прошедшее время с начала игры
			timeUp = true;
			string coinCountMessage = "SERVER_COINS:" + to_string(server_coin_count);
			int iSendResult = send(ClientSocket, coinCountMessage.c_str(), coinCountMessage.size(), 0);// Встановлюємо флаг, що час вичерпано
			if (iSendResult == SOCKET_ERROR) {
				cout << "send failed with error: " << WSAGetLastError() << "\n";
				closesocket(ClientSocket);
				WSACleanup();
				return;
			}
			break; // Вихід з циклу
		}

		// Форматуємо вивід часу і встановлюємо його у заголовок вікна
		stringstream ss;
		ss << setw(2) << setfill('0') << elapsed_minutes.count() / 60 << ":"; // Години
		ss << setw(2) << setfill('0') << elapsed_minutes.count() % 60 << ":"; // Хвилини
		ss << setw(2) << setfill('0') <<elapsed_seconds.count() % 60; // Секунды
		string timeStr = ss.str();
		SetConsoleTitleA(timeStr.c_str());

		// Затримка на 1 секунду
		this_thread::sleep_for(chrono::seconds(1));
	}

	system("cls");

	// Показати рахунок сервера та клієнта
	cout << "SERVER: " << server_coin_count << endl;
	cout << "CLIENT: " << client_coin_count << endl;

}

void UpdateClientGameTimeInWindowTitle(const string& timeStr) {
	SetConsoleTitleA(timeStr.c_str()); // Встановлюємо час у заголовок вікна
}

void GenerateMap(char**& map, const unsigned int height, const unsigned int width) {
	map = new char* [height]; // это одномерный массив указателей
	for (int y = 0; y < height; y++) // перебiр рядкiв
	{
		map[y] = new char[width]; // под каждый указатель в массиве указателей нужно выделить память под одномерный массив данных типа чар
		for (int x = 0; x < width; x++) // перебiр кожноi комiрки в межах рядка
		{
			map[y][x] = ' '; // за замовчуванням всi комiрки - це коридори 

			// тут хороше мiсце для генерацii стiн (ДЗ)
			// 
			// генеруемо скарби

			int r = rand() % 10; // 0...9
			if (r == 0)
				map[y][x] = '.';

			if (x == 0 || x == width - 1 || y == 0 || y == height - 1)
				map[y][x] = '#'; // стiнка
			if (x == width - 1 && y == height / 2)
				map[y][x] = ' '; // вихiд 
			if (x == 1 && y == 1) {
				map[y][x] = '1'; // гравець сервера
				server_smile.X = x;
				server_smile.Y = y;
			}
			if (x == 1 && y == 3) {
				map[y][x] = '2'; // гравець клiента
				client_smile.X = x;
				client_smile.Y = y;
			}
			r = rand() % 10; // генерируем случайное число от 0 до 9
			if (r == 0 && map[y][x] != '1' && map[y][x] != '2') // если случайное число равно 0 и клетка не содержит игрока, то создаем стену
				map[y][x] = '#';
		}
	}
}

void ShowMap(char** map, const unsigned int height, const unsigned int width) {
	setlocale(0, "C");
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	for (int y = 0; y < height; y++) // перебiр рядкiв
	{
		for (int x = 0; x < width; x++) // перебiр кожноi комiрки в межах рядка
		{
			if (map[y][x] == ' ')
			{
				SetConsoleTextAttribute(h, WORD(Colors::BLACK));
				cout << " ";
			}
			else if (map[y][x] == '#')
			{
				SetConsoleTextAttribute(h, WORD(Colors::DARKGREEN));
				cout << (char)219; // cout << "#"; 
			}
			else if (map[y][x] == '1')
			{
				SetConsoleTextAttribute(h, WORD(Colors::RED));
				cout << (char)160; // cout << "@";
			}
			else if (map[y][x] == '2')
			{
				SetConsoleTextAttribute(h, WORD(Colors::BLUE));
				cout << (char)160; // cout << "@";
			}
			else if (map[y][x] == '.')
			{
				SetConsoleTextAttribute(h, WORD(Colors::YELLOW));
				cout << "."; // cout << "o"; монетка
			}
		}
		cout << "\n"; // перехiд на наступний рядок
	}
}

string MakeMessage(char** map, const unsigned int height, const unsigned int width)
{
	string message = "h";
	message += to_string(height); // h5
	message += "w"; // h5w
	message += to_string(width); // h5w9
	message += "d"; // d = data
	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
			message += map[y][x];
	return message;
}

void HandleCoinCollision(char** map, int x, int y) {
	if (map[y][x] == '.') {
		if (x == server_smile.X && y == server_smile.Y) {
			server_coin_count++;
			map[y][x] = ' ';
		}
		else if (x == client_smile.X && y == client_smile.Y) {
			client_coin_count++;
			map[y][x] = ' ';
		}
	}
	
}


DWORD WINAPI Sender(void* param)
{
	// формування локацii
	unsigned int height = 15; // <<< висота рiвня в рядках
	unsigned int width = 30; // <<< ширина рiвня в стовпчиках

	char** map = nullptr; // динамiчний масив для зберiгання всiх об'ектiв локацii
	GenerateMap(map, height, width);
	system("cls");
	ShowMap(map, height, width);
	/////////////////////////
	COORD last_smile_position = server_smile;
	/////////////////////
	string message = MakeMessage(map, height, width);
	// cout << message << "\n";

	int iSendResult = send(ClientSocket, message.c_str(), message.size(), 0); // The send function sends data on a connected socket: https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-send
	if (iSendResult == SOCKET_ERROR) {
		cout << "send failed with error: " << WSAGetLastError() << "\n";
		cout << "упс, отправка (send) ответного сообщения не состоялась ((\n";
		closesocket(ClientSocket);
		WSACleanup();
		return 7;
	}

	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_CURSOR_INFO cursor;
	cursor.bVisible = false;
	cursor.dwSize = 100;
	SetConsoleCursorInfo(h, &cursor);

	auto start = chrono::system_clock::now();

	// interactive part
	while (true) {

		KeyCodes code = (KeyCodes)_getch();
		if (code == KeyCodes::ARROWS)
			code = (KeyCodes)_getch(); // если первый код 224 - то это стрелка (но пока не понятно, какая), нужно вызвать функцию ГЕЧ ещё раз для получения нормального кода 

		// стирание смайлика сервера в его "старых" координатах
		SetConsoleCursorPosition(h, server_smile);
		cout << " ";

		int direction = 0; // 1 2 3 4

		if(code == KeyCodes::LEFT && map[server_smile.Y][server_smile.X - 1] != '#') {
			server_smile.X--;
			HandleCoinCollision(map, server_smile.X, server_smile.Y);
			direction = 1;
		}
		else if (code == KeyCodes::RIGHT && map[server_smile.Y][server_smile.X + 1] != '#') {
			server_smile.X++;
			HandleCoinCollision(map, server_smile.X, server_smile.Y);
			direction = 2;
		}
		else if (code == KeyCodes::UP && map[server_smile.Y - 1][server_smile.X] != '#') {
			server_smile.Y--;
			HandleCoinCollision(map, server_smile.X, server_smile.Y);
			direction = 3;
		}
		else if (code == KeyCodes::DOWN && map[server_smile.Y + 1][server_smile.X] != '#') {
			server_smile.Y++;
			HandleCoinCollision(map, server_smile.X, server_smile.Y);
			direction = 4;
		}
		// отрисовка смайлика в его "новых" координатах
		SetConsoleCursorPosition(h, server_smile);
		SetConsoleTextAttribute(h, WORD(Colors::RED));
		cout << (char)160; // cout << "@";

		HWND hwnd = GetConsoleWindow();

		// Зміна назви вікна на "Зустрілись!" у разі зіткнення смайлів
		if ((server_smile.X == client_smile.X && server_smile.Y == client_smile.Y) ||
			(abs(server_smile.X - client_smile.X) <= 1 && abs(server_smile.Y - client_smile.Y) <= 1)) {
			system("title Meeting!!");
		}
		else {
			// Повернення стандартної назви вікна
			system("title SERVER SIDE");
		}

		

		char message[200];
		strcpy_s(message, 199, to_string(direction).c_str());
		strcat_s(message, 199, to_string(client_coin_count).c_str());
		int iResult = send(ClientSocket, message, (int)strlen(message), 0);

		if (iResult == SOCKET_ERROR) {
			cout << "send failed with error: " << WSAGetLastError() << "\n";
			closesocket(ClientSocket);
			WSACleanup();
			return 15;
		}
	}
	return 0;
}


void HandleServerCoinsMessage(const string& message) {
	// Парсим сообщение для получения количества собранных сервером точек
	size_t pos = message.find(':');
	if (pos != string::npos && pos + 1 < message.size()) {
		string coinsStr = message.substr(pos + 1);
		int serverCoins = stoi(coinsStr);
		cout << "Server collected coins: " << serverCoins << endl;
	}
}


DWORD WINAPI Receiver(void* param)
{
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

	

	while (true) {
		char answer[DEFAULT_BUFLEN];

		int iResult = recv(ClientSocket, answer, DEFAULT_BUFLEN, 0); // The recv function is used to read incoming data: https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-recv
		answer[iResult] = '\0';

		// перетворення масиву чарiв на НОРМАЛЬНИЙ рядок тексту (тобто на змiнну типу string)
		string message = answer;

		if (message.substr(0, 12) == "SERVER_COINS") {
			HandleServerCoinsMessage(message);
		}

		UpdateClientGameTimeInWindowTitle(message);

		// стирання смайла клиента в його старих координатах!
		SetConsoleCursorPosition(h, client_smile);
		SetConsoleTextAttribute(h, WORD(Colors::BLACK));
		cout << " "; // cout << "@";

		COORD temp_server_smile = server_smile;

		if (message == "1") { // client LEFT
			client_smile.X--;


		}
		else if (message == "2") { // client RIGHT
			client_smile.X++;

		}
		else if (message == "3") { // client UP
			client_smile.Y--;

		}
		else if (message == "4") { // client DOWN
			client_smile.Y++;

		}

		// показ смайла клиента в його нових координатах!
		SetConsoleCursorPosition(h, client_smile);
		SetConsoleTextAttribute(h, WORD(Colors::BLUE));
		cout << (char)160; // cout << "@";

		HWND hwnd = GetConsoleWindow();

		// Зміна назви вікна на "Зустрілись!" у разі зіткнення смайлів
		if ((server_smile.X == client_smile.X && server_smile.Y == client_smile.Y) ||
			(abs(server_smile.X - client_smile.X) <= 1 && abs(server_smile.Y - client_smile.Y) <= 1)) {
			system("title Meeting!!");
		}
		else {
			// Повернення стандартної назви вікна
			system("title SERVER SIDE");
		}

		if (iResult > 0) {
			// cout << answer << "\n";
			// cout << "байтов получено: " << iResult << "\n";
		}
		else if (iResult == 0)
			cout << "соединение с сервером закрыто.\n";
		else
			cout << "recv failed with error: " << WSAGetLastError() << "\n";
	}
	return 0;
}

void UpdateWindowTitle(HWND hwnd, const char* title) {
	SetWindowTextA(hwnd, title); // Функція для оновлення назви вікна
}

int main()
{
	setlocale(0, "");
	system("title SERVER SIDE");
	system("color 07");
	system("cls");
	MoveWindow(GetConsoleWindow(), 650, 50, 500, 500, true);
	thread timeThread(UpdateGameTimeInWindowTitle);

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	// Initialize Winsock
	WSADATA wsaData; // The WSADATA structure contains information about the Windows Sockets implementation: https://docs.microsoft.com/en-us/windows/win32/api/winsock/ns-winsock-wsadata
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData); // The WSAStartup function initiates use of the Winsock DLL by a process: https://firststeps.ru/mfc/net/socket/r.php?2
	if (iResult != 0) {
		cout << "WSAStartup failed with error: " << iResult << "\n";
		cout << "подключение Winsock.dll прошло с ошибкой!\n";
		return 1;
	}
	else {
		// cout << "подключение Winsock.dll прошло успешно!\n";
		Sleep(PAUSE);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct addrinfo hints; // The addrinfo structure is used by the getaddrinfo function to hold host address information: https://docs.microsoft.com/en-us/windows/win32/api/ws2def/ns-ws2def-addrinfoa
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET; // The Internet Protocol version 4 (IPv4) address family
	hints.ai_socktype = SOCK_STREAM; // Provides sequenced, reliable, two-way, connection-based byte streams with a data transmission mechanism
	hints.ai_protocol = IPPROTO_TCP; // The Transmission Control Protocol (TCP). This is a possible value when the ai_family member is AF_INET or AF_INET6 and the ai_socktype member is SOCK_STREAM
	hints.ai_flags = AI_PASSIVE; // The socket address will be used in a call to the "bind" function

	// Resolve the server address and port
	struct addrinfo* result = NULL;
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		cout << "getaddrinfo failed with error: " << iResult << "\n";
		cout << "получение адреса и порта сервера прошло c ошибкой!\n";
		WSACleanup(); // The WSACleanup function terminates use of the Winsock 2 DLL (Ws2_32.dll): https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsacleanup
		return 2;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Create a SOCKET for connecting to server
	SOCKET ListenSocket = INVALID_SOCKET;
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		cout << "socket failed with error: " << WSAGetLastError() << "\n";
		cout << "создание сокета прошло c ошибкой!\n";
		freeaddrinfo(result);
		WSACleanup();

		return 3;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen); // The bind function associates a local address with a socket: https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-bind
	if (iResult == SOCKET_ERROR) {
		cout << "bind failed with error: " << WSAGetLastError() << "\n";
		cout << "внедрение сокета по IP-адресу прошло с ошибкой!\n";
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 4;
	}

	freeaddrinfo(result);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	iResult = listen(ListenSocket, SOMAXCONN); // The listen function places a socket in a state in which it is listening for an incoming connection: https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-listen
	if (iResult == SOCKET_ERROR) {
		cout << "listen failed with error: " << WSAGetLastError() << "\n";
		cout << "прослушка информации от клиента не началась. что-то пошло не так!\n";
		closesocket(ListenSocket);
		WSACleanup();
		return 5;
	}
	else {
		cout << "пожалуйста, запустите client.exe\n";
		system("start D:\\ITSTEP\\OOP\\GameSmile\\x64\\Debug\\Client.exe");
		// здесь можно было бы запустить некий прелоадер в отдельном потоке
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	ClientSocket = accept(ListenSocket, NULL, NULL); // The accept function permits an incoming connection attempt on a socket: https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-accept
	if (ClientSocket == INVALID_SOCKET) {
		cout << "accept failed with error: " << WSAGetLastError() << "\n";
		cout << "соединение с клиентским приложением не установлено! печаль!\n";
		closesocket(ListenSocket);
		WSACleanup();
		return 6;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	
	CreateThread(0, 0, Receiver, 0, 0, 0);
	CreateThread(0, 0, Sender, 0, 0, 0);
	timeThread.join();

	Sleep(INFINITE);
}