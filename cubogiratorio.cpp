// Cubo em CxxDroid - gradiente de cor animado + iluminação com brilho
// + sombra centralizada no fundo + giro que muda de direção sozinho + áudio em loop
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <GLES/gl.h>
#include <math.h>
#include <cstdlib>
#include <ctime>

// ---------------------------------------------------------
// Geometria do cubo (vértices originais)
// ---------------------------------------------------------
GLfloat vertices[] = {
	-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
	-0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f,
	-0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f,
	0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f,
	-0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f,
	-0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f};

// Normais por face (necessárias para a iluminação/brilho).
GLfloat normais[] = {
	 0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f, // frente
	 0.0f,  0.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f, -1.0f, // trás
	-1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f, // esquerda
	 1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f, // direita
	 0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f, // topo
	 0.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f  // base
};

GLubyte indices[] = {
	0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7,
	8, 9, 10, 8, 10, 11, 12, 13, 14, 12, 14, 15,
	16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23};

// Cor de cada um dos 24 vértices (R,G,B,A). É recalculada a cada quadro
// para criar o efeito de gradiente animado sobre as faces do cubo.
GLfloat cores_cubo[24 * 4];

// ---------------------------------------------------------
// Parede de fundo (backdrop único) que recebe a sombra
// ---------------------------------------------------------
GLfloat parede_fundo_vertices[] = {
	-6.0f, -6.0f, -9.0f,
	 6.0f, -6.0f, -9.0f,
	 6.0f,  6.0f, -9.0f,
	-6.0f,  6.0f, -9.0f
};

// Equação do plano da parede (a, b, c, d) para ax+by+cz+d=0
GLfloat plano_fundo[4] = {0.0f, 0.0f, 1.0f, 9.0f};

// Luz usada para ILUMINAR o cubo (dá o brilho/highlight visível nas faces)
GLfloat luz_pos[4] = {1.5f, 3.5f, -2.0f, 1.0f};

// Luz "direcional" usada só para CALCULAR A SOMBRA.
// Ser direcional (w=0, como o sol) evita que a sombra "dispare" para
// longe do cubo - ela fica sempre centralizada e logo abaixo dele,
// não importa onde o cubo esteja. Ajuste ly/lz para mudar o ângulo:
// mais ly = sombra mais próxima do cubo; menos ly = sombra mais distante.
GLfloat luz_sombra[4] = {0.0f, 1.3f, 3.0f, 0.0f};

// ---------------------------------------------------------
// Volume da música de fundo.
// Vai de 0 (mudo) até 128 (MIX_MAX_VOLUME, volume máximo).
// Mude esse número para controlar o volume via código.
// ---------------------------------------------------------
int volume_musica = 45; // metade do volume máximo

// ---------------------------------------------------------
// CONTROLE DE VELOCIDADE DO CUBO — mexa aqui para testar.
// 10 = velocidade padrão/normal (a mesma de sempre).
// Vai subindo até 100 = 10x mais rápido que o normal.
// Pode usar valores fora dessa faixa também se quiser.
// ---------------------------------------------------------
int velocidade_percentual = 40;

// ---------------------------------------------------------
// Controle do giro do cubo em várias direções.
// Em vez de girar sempre no mesmo eixo, o cubo troca de eixo de
// tempos em tempos e faz uma transição suave até a nova direção.
// ---------------------------------------------------------
GLfloat matriz_rotacao_cubo[16] = { // matriz acumulada do giro (começa "parada")
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
};
float eixo_atual[3] = {1.0f, 1.0f, 0.5f}; // eixo de giro usado neste instante
float eixo_alvo[3]  = {1.0f, 1.0f, 0.5f}; // eixo para onde estamos transicionando
float velocidade_rot = 1.0f;              // graus de giro por quadro
Uint32 proxima_troca_eixo = 0;            // quando (em ms) sorteamos um novo eixo

// ---------------------------------------------------------
// Monta a matriz de sombra (projeção do cubo sobre um plano,
// vista a partir de uma fonte de luz)
// ---------------------------------------------------------
void montar_matriz_sombra(GLfloat m[16], const GLfloat plano[4], const GLfloat luz[4])
{
	GLfloat dot = plano[0]*luz[0] + plano[1]*luz[1] + plano[2]*luz[2] + plano[3]*luz[3];

	m[0]  = dot - luz[0]*plano[0];
	m[4]  =     - luz[0]*plano[1];
	m[8]  =     - luz[0]*plano[2];
	m[12] =     - luz[0]*plano[3];

	m[1]  =     - luz[1]*plano[0];
	m[5]  = dot - luz[1]*plano[1];
	m[9]  =     - luz[1]*plano[2];
	m[13] =     - luz[1]*plano[3];

	m[2]  =     - luz[2]*plano[0];
	m[6]  =     - luz[2]*plano[1];
	m[10] = dot - luz[2]*plano[2];
	m[14] =     - luz[2]*plano[3];

	m[3]  =     - luz[3]*plano[0];
	m[7]  =     - luz[3]*plano[1];
	m[11] =     - luz[3]*plano[2];
	m[15] = dot - luz[3]*plano[3];
}

// ---------------------------------------------------------
// Converte HSV -> RGB (para animar a cor do cubo com o tempo)
// ---------------------------------------------------------
void hsv_para_rgb(float h, float s, float v, float *r, float *g, float *b)
{
	float c = v * s;
	float hh = fmodf(h / 60.0f, 6.0f);
	float x = c * (1.0f - fabsf(fmodf(hh, 2.0f) - 1.0f));
	float m = v - c;
	float rr, gg, bb;

	if (hh < 1.0f)      { rr = c; gg = x; bb = 0; }
	else if (hh < 2.0f) { rr = x; gg = c; bb = 0; }
	else if (hh < 3.0f) { rr = 0; gg = c; bb = x; }
	else if (hh < 4.0f) { rr = 0; gg = x; bb = c; }
	else if (hh < 5.0f) { rr = x; gg = 0; bb = c; }
	else                { rr = c; gg = 0; bb = x; }

	*r = rr + m;
	*g = gg + m;
	*b = bb + m;
}

// Normaliza um vetor 3D (deixa o comprimento dele = 1).
// Necessário para o eixo de rotação não "acelerar" o giro quando
// os componentes sorteados forem maiores.
void normalizar_vetor(float v[3])
{
	float comprimento = sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	if (comprimento > 0.0001f)
	{
		v[0] /= comprimento;
		v[1] /= comprimento;
		v[2] /= comprimento;
	}
}

// Aplica só a parte de rotação (3x3) de uma matriz 4x4 no formato do OpenGL
// (column-major) a um vetor. Usamos para girar as normais do cubo junto com
// ele, já que agora calculamos a luz na mão em vez de deixar o OpenGL fazer.
void rotacionar_vetor(const GLfloat m[16], const float v[3], float saida[3])
{
	saida[0] = m[0]*v[0] + m[4]*v[1] + m[8]*v[2];
	saida[1] = m[1]*v[0] + m[5]*v[1] + m[9]*v[2];
	saida[2] = m[2]*v[0] + m[6]*v[1] + m[10]*v[2];
}

// ---------------------------------------------------------
// Controle de toque (touch screen):
// - arrastar 1 dedo gira o cubo manualmente, seguindo o dedo;
// - cada toque dá um pequeno "empurrão" de velocidade, que some aos poucos;
// - usar 2 dedos (pinça) dá zoom no cubo; soltar volta tudo ao normal.
// ---------------------------------------------------------
struct DedoInfo
{
	bool ativo = false;
	SDL_FingerID id = 0;
	float x = 0.0f, y = 0.0f; // posição normalizada (0.0 a 1.0) na tela
};
DedoInfo dedos[4]; // rastreia até 4 dedos ao mesmo tempo na tela

bool arrastando = false;       // true = 1 dedo controlando a rotação manualmente
SDL_FingerID dedo_arrasto = 0; // id do dedo que está arrastando agora
float toque_x_ant = 0.0f, toque_y_ant = 0.0f; // última posição usada do arrasto

bool em_pinca = false; // true = 2+ dedos fazendo o gesto de zoom (pinça)
SDL_FingerID pinca_dedo_a = 0, pinca_dedo_b = 0;
float pinca_dist_inicial = 0.0f; // distância entre os 2 dedos quando a pinça começou
float pinca_zoom_inicial = 1.0f; // zoom que já estava valendo quando a pinça começou

float zoom_atual = 1.0f; // fator de escala aplicado ao cubo agora (suavizado)
float zoom_alvo  = 1.0f; // fator para onde o zoom está indo (1.0 = tamanho normal)

float velocidade_extra = 0.0f;          // "empurrão" de velocidade dado pelos toques
const float TOQUE_IMPULSO       = 0.9f; // quanto cada toque acelera o giro
const float TOQUE_IMPULSO_MAX   = 6.0f; // limite do empurrão acumulado
const float TOQUE_DECAIMENTO    = 0.985f; // taxa com que o empurrão some (por quadro)

const float SENSIBILIDADE_ARRASTO = 320.0f; // graus de giro por "tela inteira" arrastada
const float ZOOM_MIN = 0.5f; // quanto o cubo pode encolher no "beliscão" para fora
const float ZOOM_MAX = 2.2f; // quanto o cubo pode crescer no "beliscão" para dentro

// Acha o slot de um dedo pelo id do SDL. Devolve -1 se não achar.
int achar_dedo(SDL_FingerID id)
{
	for (int i = 0; i < 4; i++)
		if (dedos[i].ativo && dedos[i].id == id) return i;
	return -1;
}

// Acha um slot livre para guardar um dedo novo. Devolve -1 se não houver.
int achar_slot_livre()
{
	for (int i = 0; i < 4; i++)
		if (!dedos[i].ativo) return i;
	return -1;
}

int contar_dedos_ativos()
{
	int n = 0;
	for (int i = 0; i < 4; i++) if (dedos[i].ativo) n++;
	return n;
}

// Distância (em pixels reais da tela) entre dois dedos. Precisa multiplicar
// pela largura/altura porque as coordenadas do SDL vêm normalizadas (0..1).
float distancia_entre_dedos(const DedoInfo &a, const DedoInfo &b, int largura_tela, int altura_tela)
{
	float dx = (a.x - b.x) * largura_tela;
	float dy = (a.y - b.y) * altura_tela;
	return sqrtf(dx * dx + dy * dy);
}

// Reavalia, a cada dedo que toca ou levanta, se devemos estar arrastando
// (1 dedo) ou fazendo a pinça de zoom (2 dedos ou mais).
void atualizar_modo_toque(int largura_tela, int altura_tela)
{
	int n = contar_dedos_ativos();

	if (n == 0)
	{
		arrastando = false;
		em_pinca = false;
		zoom_alvo = 1.0f; // ninguém tocando -> zoom volta ao normal
		return;
	}

	if (n == 1)
	{
		em_pinca = false;
		zoom_alvo = 1.0f; // só sobrou 1 dedo -> zoom volta ao normal

		for (int i = 0; i < 4; i++)
		{
			if (dedos[i].ativo)
			{
				if (!arrastando || dedo_arrasto != dedos[i].id)
				{
					// Começando um arrasto novo: guarda a posição para não "pular"
					dedo_arrasto = dedos[i].id;
					toque_x_ant = dedos[i].x;
					toque_y_ant = dedos[i].y;
				}
				arrastando = true;
				break;
			}
		}
		return;
	}

	// n >= 2: modo pinça, usando os 2 primeiros dedos ativos encontrados
	arrastando = false;
	int primeiro = -1, segundo = -1;
	for (int i = 0; i < 4 && segundo == -1; i++)
	{
		if (dedos[i].ativo)
		{
			if (primeiro == -1) primeiro = i;
			else segundo = i;
		}
	}

	bool pinca_nova = !em_pinca || pinca_dedo_a != dedos[primeiro].id || pinca_dedo_b != dedos[segundo].id;
	if (pinca_nova)
	{
		pinca_dedo_a = dedos[primeiro].id;
		pinca_dedo_b = dedos[segundo].id;
		pinca_dist_inicial = distancia_entre_dedos(dedos[primeiro], dedos[segundo], largura_tela, altura_tela);
		if (pinca_dist_inicial < 1.0f) pinca_dist_inicial = 1.0f;
		pinca_zoom_inicial = zoom_atual;
	}
	em_pinca = true;
}

int main(int argc, char *argv[])
{
	srand((unsigned int)time(NULL)); // semente para sortear novas direções de giro

	// Inicializa vídeo E áudio
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

	SDL_Window *window = SDL_CreateWindow("Cubo Colorido",
										  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
										  720, 1280, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

	SDL_GLContext context = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(1);

	// ---------- Áudio: abre o dispositivo e carrega a música de fundo ----------
	Mix_Music *musica_fundo = NULL;
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == 0)
	{
		musica_fundo = Mix_LoadMUS("fundoloop.wav");
		if (musica_fundo != NULL)
		{
			Mix_VolumeMusic(volume_musica); // aplica o volume antes de tocar
			Mix_PlayMusic(musica_fundo, -1); // -1 = repete para sempre
		}
		else
		{
			SDL_Log("Nao foi possivel carregar fundoloop.wav: %s", Mix_GetError());
		}
	}
	else
	{
		SDL_Log("Nao foi possivel abrir o audio: %s", Mix_GetError());
	}

	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	glViewport(0, 0, w, h);

	// Pinta a tela de cinza IMEDIATAMENTE (evita o flash branco da splash)
	for (int i = 0; i < 2; i++)
	{
		glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		SDL_GL_SwapWindow(window);
	}

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_NORMALIZE);

	// ---------- Configuração de iluminação (dá o "brilho" no cubo) ----------
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	GLfloat luz_ambiente[]  = {0.15f, 0.15f, 0.15f, 1.0f};
	GLfloat luz_difusa[]    = {0.85f, 0.85f, 0.85f, 1.0f};
	GLfloat luz_especular[] = {1.0f, 1.0f, 1.0f, 1.0f};

	glLightfv(GL_LIGHT0, GL_AMBIENT, luz_ambiente);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, luz_difusa);
	glLightfv(GL_LIGHT0, GL_SPECULAR, luz_especular);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, luz_ambiente);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float ratio = (float)w / h;
	glFrustumf(-ratio, ratio, -1, 1, 2, 10);

	// Pré-calcula a matriz de sombra (plano fixo, luz-sombra fixa)
	GLfloat matriz_sombra[16];
	montar_matriz_sombra(matriz_sombra, plano_fundo, luz_sombra);

	bool rodando = true;
	SDL_Event evento;

	while (rodando)
	{
		while (SDL_PollEvent(&evento))
		{
			switch (evento.type)
			{
			case SDL_QUIT:
				rodando = false;
				break;

			case SDL_FINGERDOWN:
			{
				bool primeiro_dedo_da_tela = (contar_dedos_ativos() == 0);

				int slot = achar_slot_livre();
				if (slot != -1)
				{
					dedos[slot].ativo = true;
					dedos[slot].id = evento.tfinger.fingerId;
					dedos[slot].x = evento.tfinger.x;
					dedos[slot].y = evento.tfinger.y;
				}

				// "A cada toque" dá um empurrão de velocidade - só quando é o
				// primeiro dedo a tocar a tela, para não somar de novo ao
				// começar uma pinça de 2 dedos.
				if (primeiro_dedo_da_tela)
				{
					velocidade_extra += TOQUE_IMPULSO;
					if (velocidade_extra > TOQUE_IMPULSO_MAX) velocidade_extra = TOQUE_IMPULSO_MAX;
				}

				atualizar_modo_toque(w, h);
				break;
			}

			case SDL_FINGERUP:
			{
				int slot = achar_dedo(evento.tfinger.fingerId);
				if (slot != -1) dedos[slot].ativo = false;
				atualizar_modo_toque(w, h);
				break;
			}

			case SDL_FINGERMOTION:
			{
				int slot = achar_dedo(evento.tfinger.fingerId);
				if (slot != -1)
				{
					dedos[slot].x = evento.tfinger.x;
					dedos[slot].y = evento.tfinger.y;
				}

				if (arrastando && evento.tfinger.fingerId == dedo_arrasto)
				{
					float dx = evento.tfinger.x - toque_x_ant;
					float dy = evento.tfinger.y - toque_y_ant;

					float angulo_y =  dx * SENSIBILIDADE_ARRASTO; // arrasto horizontal -> gira no eixo Y
					float angulo_x = -dy * SENSIBILIDADE_ARRASTO; // arrasto vertical   -> gira no eixo X

					// Aplica a rotação do arrasto em coordenadas de tela (mundo),
					// "por cima" da rotação já acumulada, para o cubo seguir o
					// dedo de forma natural, não importa a direção.
					glLoadIdentity();
					glRotatef(angulo_x, 1.0f, 0.0f, 0.0f);
					glRotatef(angulo_y, 0.0f, 1.0f, 0.0f);
					glMultMatrixf(matriz_rotacao_cubo);
					glGetFloatv(GL_MODELVIEW_MATRIX, matriz_rotacao_cubo);
					glLoadIdentity();

					toque_x_ant = evento.tfinger.x;
					toque_y_ant = evento.tfinger.y;
				}
				else if (em_pinca)
				{
					int slot_a = achar_dedo(pinca_dedo_a);
					int slot_b = achar_dedo(pinca_dedo_b);
					if (slot_a != -1 && slot_b != -1)
					{
						float dist_atual = distancia_entre_dedos(dedos[slot_a], dedos[slot_b], w, h);
						zoom_alvo = pinca_zoom_inicial * (dist_atual / pinca_dist_inicial);
						if (zoom_alvo < ZOOM_MIN) zoom_alvo = ZOOM_MIN;
						if (zoom_alvo > ZOOM_MAX) zoom_alvo = ZOOM_MAX;
					}
				}
				break;
			}

			default:
				break;
			}
		}

		glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		// ---------- Impulso de velocidade dos toques: some aos poucos ----------
		velocidade_extra *= TOQUE_DECAIMENTO;
		if (velocidade_extra < 0.01f) velocidade_extra = 0.0f;

		// ---------- Zoom da pinça: sempre suaviza até o valor alvo (volta ao normal ao soltar) ----------
		zoom_atual += (zoom_alvo - zoom_atual) * 0.12f;

		// ---------- Atualiza o giro do cubo (automático, OU manual enquanto o dedo arrasta) ----------
		if (!arrastando)
		{
			// Troca de direção de tempos em tempos
			Uint32 agora = SDL_GetTicks();
			if (agora >= proxima_troca_eixo)
			{
				// Sorteia uma nova direção de giro (cada componente entre -1.0 e 1.0)
				eixo_alvo[0] = (rand() % 200 - 100) / 100.0f;
				eixo_alvo[1] = (rand() % 200 - 100) / 100.0f;
				eixo_alvo[2] = (rand() % 200 - 100) / 100.0f;
				normalizar_vetor(eixo_alvo);

				float fator_velocidade = velocidade_percentual / 10.0f; // 10 -> 1.0x (velocidade normal), 100 -> 10.0x
				velocidade_rot = (0.6f + (rand() % 150) / 100.0f) * fator_velocidade; // base 0.6 a 2.1 graus/quadro, escalada
				proxima_troca_eixo = agora + 3000 + (rand() % 3000); // nova troca em 3 a 6s
			}

			// Transição suave do eixo atual até o eixo sorteado (evita giro "quebrado")
			eixo_atual[0] += (eixo_alvo[0] - eixo_atual[0]) * 0.02f;
			eixo_atual[1] += (eixo_alvo[1] - eixo_atual[1]) * 0.02f;
			eixo_atual[2] += (eixo_alvo[2] - eixo_atual[2]) * 0.02f;
			normalizar_vetor(eixo_atual);

			// Acumula mais um passo de giro na matriz de rotação do cubo
			// (velocidade_rot + velocidade_extra: o impulso dos toques acelera o giro normal)
			glLoadIdentity();
			glMultMatrixf(matriz_rotacao_cubo);
			glRotatef(velocidade_rot + velocidade_extra, eixo_atual[0], eixo_atual[1], eixo_atual[2]);
			glGetFloatv(GL_MODELVIEW_MATRIX, matriz_rotacao_cubo);
			glLoadIdentity();
		}
		// Enquanto "arrastando" é true, a rotação já foi aplicada direto no
		// SDL_FINGERMOTION acima, seguindo o dedo do usuário. Ao soltar, o
		// giro automático retoma sozinho de onde o cubo ficou.

		// A luz do cubo é definida em coordenadas "de mundo"
		glLightfv(GL_LIGHT0, GL_POSITION, luz_pos);

		glEnableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);

		// ---------- 1) Desenha a parede de fundo (cor sólida, sem luz) ----------
		glDisable(GL_LIGHTING);
		glColor4f(0.32f, 0.32f, 0.32f, 1.0f);
		glVertexPointer(3, GL_FLOAT, 0, parede_fundo_vertices);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		// ---------- 2) Desenha a sombra do cubo, centralizada, logo abaixo ----------
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(-2.0f, -2.0f); // evita z-fighting com a parede
		glColor4f(0.0f, 0.0f, 0.0f, 0.5f);

		glVertexPointer(3, GL_FLOAT, 0, vertices);

		glPushMatrix();
		glMultMatrixf(matriz_sombra);
		glTranslatef(0.0f, 0.0f, -5.0f);
		glScalef(zoom_atual, zoom_atual, zoom_atual); // acompanha o zoom da pinça
		glMultMatrixf(matriz_rotacao_cubo);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, indices);
		glPopMatrix();

		glDisable(GL_POLYGON_OFFSET_FILL);
		glDisable(GL_BLEND);
		glEnable(GL_LIGHTING);

		// ---------- 3) Desenha o cubo "real", com gradiente de cor animado e brilho ----------
		// Este aparelho não tem a função glColorMaterial, então a iluminação do
		// cubo é calculada aqui na mão (por vértice) e desenhada com a luz do
		// OpenGL desligada — assim a cor calculada não é substituída por ela.
		float hue_base = fmodf(SDL_GetTicks() * 0.05f, 360.0f); // velocidade da troca de cor

		// Direção aproximada da luz (do centro do cubo até a posição da luz)
		float dir_luz[3] = {
			luz_pos[0] - 0.0f,
			luz_pos[1] - 0.0f,
			luz_pos[2] - (-5.0f)
		};
		normalizar_vetor(dir_luz);

		// Vetor "de meio-caminho" entre a luz e a câmera, usado para o brilho especular
		float dir_camera[3] = {0.0f, 0.0f, 1.0f}; // a câmera olha para -Z
		float meio_vetor[3] = {
			dir_luz[0] + dir_camera[0],
			dir_luz[1] + dir_camera[1],
			dir_luz[2] + dir_camera[2]
		};
		normalizar_vetor(meio_vetor);

		// Calcula a cor final de cada vértice: gradiente animado + luz + brilho
		for (int i = 0; i < 24; i++)
		{
			float px = vertices[i * 3 + 0];
			float py = vertices[i * 3 + 1];
			float pz = vertices[i * 3 + 2];
			float fator = (px + py + pz + 1.5f) / 3.0f; // 0.0 a 1.0 conforme a posição

			float rb, gb, bb; // cor base do gradiente (antes da luz)
			hsv_para_rgb(fmodf(hue_base + fator * 150.0f, 360.0f), 0.85f, 1.0f, &rb, &gb, &bb);

			// Gira a normal do vértice junto com o cubo, para a luz acompanhar o giro
			float normal_girada[3];
			rotacionar_vetor(matriz_rotacao_cubo, &normais[i * 3], normal_girada);

			float difusa = normal_girada[0]*dir_luz[0] + normal_girada[1]*dir_luz[1] + normal_girada[2]*dir_luz[2];
			if (difusa < 0.0f) difusa = 0.0f;

			float especular = normal_girada[0]*meio_vetor[0] + normal_girada[1]*meio_vetor[1] + normal_girada[2]*meio_vetor[2];
			if (especular < 0.0f) especular = 0.0f;
			especular = powf(especular, 64.0f); // quanto maior o expoente, mais concentrado o brilho

			float sombreamento = 0.15f + 0.85f * difusa; // luz ambiente + luz difusa

			float r = rb * sombreamento + especular;
			float g = gb * sombreamento + especular;
			float b = bb * sombreamento + especular;

			cores_cubo[i * 4 + 0] = (r > 1.0f) ? 1.0f : r;
			cores_cubo[i * 4 + 1] = (g > 1.0f) ? 1.0f : g;
			cores_cubo[i * 4 + 2] = (b > 1.0f) ? 1.0f : b;
			cores_cubo[i * 4 + 3] = 1.0f;
		}

		glDisable(GL_LIGHTING); // a luz já foi calculada na mão acima
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_FLOAT, 0, cores_cubo);
		glVertexPointer(3, GL_FLOAT, 0, vertices);

		glPushMatrix();
		glTranslatef(0.0f, 0.0f, -5.0f);
		glScalef(zoom_atual, zoom_atual, zoom_atual); // efeito de zoom da pinça
		glMultMatrixf(matriz_rotacao_cubo);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, indices);
		glPopMatrix();

		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);

		SDL_GL_SwapWindow(window);
		SDL_Delay(16);
	}

	// ---------- Libera o áudio ----------
	if (musica_fundo != NULL)
	{
		Mix_FreeMusic(musica_fundo);
	}
	Mix_CloseAudio();

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}