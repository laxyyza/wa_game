# wa_game
WA Game - A Simple Multiplayer Game.

wa_game is a simple server-client multiplayer project designed for educational purposes. The name serves as a placeholder, as the client is built upon my previous project, wa_opengl, which tested OpenGL functionalities within my Window Abstraction (WA) Library.

This project is primarily developed from the ground up, without the use of a game engine or windowing library. It directly utilizes OpenGL along with the operating system’s APIs to create windows—specifically Win32 for Windows and Wayland for Linux. Additionally, it incorporates my own protocol and networking library.

One of the primary goals of this project is to implement my custom protocol, SSP (Simple Segmented Protocol), within an application. SSP is designed to send multiple different types of data in a single packet, with each type of data being buffered before serialization and transmission. Other objectives include enhancing my skills in multiplayer networking, deepening my understanding of OpenGL, and gaining experience with Win32 and Wayland.

This project (wa_game) is entirely written in C.

Features:
- Utilizes OpenGL for rendering.
- Creates windows directly using Wayland on Linux and Win32 on Windows.
- Implements my custom protocol and networking library (SSP), tailored for multiplayer games.

Server: Operates as a non-sleeping, fixed-tickrate server.
