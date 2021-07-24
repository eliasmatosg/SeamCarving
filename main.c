#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Para usar strings
#include <limits.h>

#ifdef WIN32
#include <windows.h> // Apenas para Windows
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>   // Funções da OpenGL
#include <GL/glu.h>  // Funções da GLU
#include <GL/glut.h> // Funções da FreeGLUT
#endif

// SOIL é a biblioteca para leitura das imagens
#include <SOIL.h>

// Um pixel RGB (24 bits)
typedef struct
{
    unsigned char r, g, b;
} RGB8;
//Não pode usar usar um atributo energia pois RGB8 é o pixel de entrada


// Uma imagem RGB
typedef struct
{
    int width, height;
    RGB8 *img;
} Img;

// Protótipos
void load(char *name, Img *pic);
void uploadTexture();
void seamcarve(int targetWidth); // executa o algoritmo
void freemem();                  // limpa memória (caso tenha alocado dinamicamente)

// Funções da interface gráfica e OpenGL
void init();
void draw();
void keyboard(unsigned char key, int x, int y);
void arrow_keys(int a_keys, int x, int y);

// Largura e altura da janela
int width, height;

// Largura desejada (selecionável)
int targetW;


// Primeira execução
int isFirstExecution = 1;

// Identificadores de textura
GLuint tex[3];

// As 3 imagens
Img pic[3];
Img *source;
Img *mask;
Img *target;

// Imagem selecionada (0,1,2)
int sel;

// Carrega uma imagem para a struct Img
void load(char *name, Img *pic)
{
    int chan;
    pic->img = (RGB8 *)SOIL_load_image(name, &pic->width, &pic->height, &chan, SOIL_LOAD_RGB);
    if (!pic->img)
    {
        printf("SOIL loading error: '%s'\n", SOIL_last_result());
        exit(1);
    }
    printf("Load: %d x %d x %d\n", pic->width, pic->height, chan);
}

// Implemente AQUI o seu algoritmo
void seamcarve(int targetWidth)
{
    // Aplica o algoritmo e gera a saida em target->img...
    //colunas                       linhas
    RGB8(*ptrTarget)
    [target->width] = (RGB8(*)[target->width])target->img;
    RGB8(*ptrSource)
    [source->width] = (RGB8(*)[source->width])source->img;
    RGB8(*ptrMask)
    [mask->width] = (RGB8(*)[mask->width])mask->img;

    //Matriz de energias
    long (*matEnergias)
    [targetWidth] = malloc(target->height * sizeof *matEnergias);
    
    //Target recebe Source na primeira execução do algoritmo e zerando a matriz de energias
    if (isFirstExecution){
        for (int y = 0; y < source->height; y++){
            for (int x = 0; x < source->width; x++){
                ptrTarget[y][x].r = ptrSource[y][x].r;
                ptrTarget[y][x].g = ptrSource[y][x].g;
                ptrTarget[y][x].b = ptrSource[y][x].b; 
                matEnergias[y][x] = 0;
            }
        }
        isFirstExecution = 0;
    }

    for (int y = 0; y < target->height; y++)
    {
        for (int x = 0; x < targetW; x++){
            int xMais1 = x+1;
            int xMenos1 = x-1;
            int yMais1 = y+1;
            int yMenos1 = y-1;

            if (y == 0){
                yMenos1 = target->height-1;
            }
            else if(y == target->height-1){
                yMais1 = 0;
            }
            if (x == 0 ){
                xMenos1 = targetW-1;
            }
            else if (x == targetW-1){
                xMais1 = 0;
            }          

            // Cálculo da energia de cada pixel 
            long deltaRx = (ptrTarget[y][xMais1].r) - (ptrTarget[y][xMenos1].r);
            long deltaGx = (ptrTarget[y][xMais1].g) - (ptrTarget[y][xMenos1].g);
            long deltaBx = (ptrTarget[y][xMais1].b) - (ptrTarget[y][xMenos1].b);
            long deltaX = deltaRx*deltaRx + deltaGx*deltaGx + deltaBx*deltaBx;

            long deltaRy = (ptrTarget[yMais1][x].r) - (ptrTarget[yMenos1][x].r);
            long deltaGy = (ptrTarget[yMais1][x].g) - (ptrTarget[yMenos1][x].g);
            long deltaBy = (ptrTarget[yMais1][x].b) - (ptrTarget[yMenos1][x].b);
            long deltaY = deltaRy*deltaRy + deltaGy*deltaGy + deltaBy*deltaBy;

            matEnergias[y][x] = deltaX + deltaY; 
        }
        for(int x=targetW; x<target->width; x++){
            ptrTarget[y][x].r = ptrTarget[y][x].g = ptrTarget[y][x].b = 0;
        }
    }

    //APLICAÇÃO DA MÁSCARA 
    for (int y = 0; y < target->height; y++){
        for (int x = 0; x < targetW; x++){
            if (ptrMask[y][x].g <= 30 && ptrMask[y][x].b <= 30 && ptrMask[y][x].r >= 170){ //Aqui o pixel é VERMELHO
                 matEnergias[y][x] = -800000;
            }
            else if(ptrMask[y][y].b <= 30 && ptrMask[y][x].r <= 30 && ptrMask[y][x].g >= 170){ //Aqui o pixel é verde
                 matEnergias[y][x] = 800000;
            }
        }
    }


    //CALCULO DA MATRIZ ACUMULADA
    long aux;
    for (int y=1; y<target->height; y++){
        for (int x=0; x<targetW; x++){
            if (x == 0){
                (matEnergias[y-1][x] < matEnergias[y-1][x+1]) ? (matEnergias[y][x] += matEnergias[y-1][x]) : (matEnergias[y][x] += matEnergias[y-1][x+1]);
            }
            else if (x == targetW-1){
                (matEnergias[y-1][x] < matEnergias[y-1][x-1]) ? (matEnergias[y][x] += matEnergias[y-1][x]) : (matEnergias[y][x] += matEnergias[y-1][x-1]);
            }
            else{
                (matEnergias[y-1][x] < matEnergias[y-1][x-1]) ? (aux = matEnergias[y-1][x]) : (aux = matEnergias[y-1][x-1]);
                (aux < matEnergias[y-1][x+1]) ? (matEnergias[y][x] += aux) : (matEnergias[y][x] += matEnergias[y-1][x+1]);
            }
        }
    }

    //LAÇO PARA PEGAR A POSIÇÃO DO MENOR VALOR DE ENERGIA ACUMULADO
    int y = target->height-1; 
    int xMinEnergia = 0;
    for(int x=1; x<targetW; x++){
        if(matEnergias[y][x]<matEnergias[y][xMinEnergia])
            xMinEnergia = x;
    }
    //Retirando o pixel 
    for(int x = xMinEnergia; x <targetW-1;x++)
        ptrTarget[y][x] = ptrTarget[y][x+1];


    //SUBIDA NA MATRIZ ESCOLHENDO O MENOR CAMINHO E AJUSTANDO OS PIXELS
    for (int y = target->height-2; y >= 0 ; y--){
        //Se o pixel for o mais a esquerda
        if (xMinEnergia == 0){
            matEnergias[y][xMinEnergia] < matEnergias[y][xMinEnergia+1] ? (xMinEnergia = xMinEnergia) : (xMinEnergia = xMinEnergia+1); 
            for(int x = xMinEnergia; x <targetW-1;x++){
               ptrTarget[y][x] = ptrTarget[y][x+1]; 
               ptrMask[y][x] = ptrMask[y][x+1]; //MÁSCARA ACOMPANHA A IMAGEM CONFORME ELA É CORTADA
            }
        }
        //se o pixel for o mais a direita
        else if(xMinEnergia == targetW-1){
            matEnergias[y][xMinEnergia] < matEnergias[y][xMinEnergia-1] ? (xMinEnergia = xMinEnergia) : (xMinEnergia = xMinEnergia-1); 
            for(int x = xMinEnergia;x<targetW-1;x++){
               ptrTarget[y][x] = ptrTarget[y][x+1]; 
               ptrMask[y][x] = ptrMask[y][x+1]; //MÁSCARA ACOMPANHA A IMAGEM CONFORME ELA É CORTADA
            }
        }
        else{
            matEnergias[y][xMinEnergia] < matEnergias[y][xMinEnergia+1] ? (xMinEnergia = xMinEnergia) : (xMinEnergia = xMinEnergia +1);
            matEnergias[y][xMinEnergia] < matEnergias[y][xMinEnergia-1] ? (xMinEnergia = xMinEnergia) : (xMinEnergia = xMinEnergia-1);
            for(int x = xMinEnergia;x<targetW-1;x++) {  
               ptrTarget[y][x] = ptrTarget[y][x+1]; 
               ptrMask[y][x] = ptrMask[y][x+1]; //MÁSCARA ACOMPANHA A IMAGEM CONFORME ELA É CORTADA
            }
        }
    }

    free(matEnergias);
    uploadTexture();
    glutPostRedisplay();
}

void freemem(){
    // Libera a memória ocupada pelas 3 imagens
    free(pic[0].img);
    free(pic[1].img);
    free(pic[2].img);
}

/********************************************************************
 * 
 *  VOCÊ NÃO DEVE ALTERAR NADA NO PROGRAMA A PARTIR DESTE PONTO!
 *
 ********************************************************************/
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("seamcarving [origem] [mascara]\n");
        printf("Origem é a imagem original, mascara é a máscara desejada\n");
        exit(1);
    }
    glutInit(&argc, argv);

    // Define do modo de operacao da GLUT
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    // pic[0] -> imagem original
    // pic[1] -> máscara desejada
    // pic[2] -> resultado do algoritmo

    // Carrega as duas imagens
    load(argv[1], &pic[0]);
    load(argv[2], &pic[1]);

    if (pic[0].width != pic[1].width || pic[0].height != pic[1].height)
    {
        printf("Imagem e máscara com dimensões diferentes!\n");
        exit(1);
    }

    // A largura e altura da janela são calculadas de acordo com a maior
    // dimensão de cada imagem
    width = pic[0].width;
    height = pic[0].height;

    // A largura e altura da imagem de saída são iguais às da imagem original (1)
    pic[2].width = pic[1].width;
    pic[2].height = pic[1].height;

    // Ponteiros para as structs das imagens, para facilitar
    source = &pic[0];
    mask = &pic[1];
    target = &pic[2];

    // Largura desejada inicialmente é a largura da janela
    targetW = target->width;

    // Especifica o tamanho inicial em pixels da janela GLUT
    glutInitWindowSize(width, height);

    // Cria a janela passando como argumento o titulo da mesma
    glutCreateWindow("Seam Carving");

    // Registra a funcao callback de redesenho da janela de visualizacao
    glutDisplayFunc(draw);

    // Registra a funcao callback para tratamento das teclas ASCII
    glutKeyboardFunc(keyboard);

    // Registra a funcao callback para tratamento das setas
    glutSpecialFunc(arrow_keys);

    // Cria texturas em memória a partir dos pixels das imagens
    tex[0] = SOIL_create_OGL_texture((unsigned char *)pic[0].img, pic[0].width, pic[0].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);
    tex[1] = SOIL_create_OGL_texture((unsigned char *)pic[1].img, pic[1].width, pic[1].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

    // Exibe as dimensões na tela, para conferência
    printf("Origem  : %s %d x %d\n", argv[1], pic[0].width, pic[0].height);
    printf("Máscara : %s %d x %d\n", argv[2], pic[1].width, pic[0].height);
    sel = 0; // pic1

    // Define a janela de visualizacao 2D
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(0.0, width, height, 0.0);
    glMatrixMode(GL_MODELVIEW);

    // Aloca memória para a imagem de saída
    pic[2].img = malloc(pic[1].width * pic[1].height * 3); // W x H x 3 bytes (RGB)
    // Pinta a imagem resultante de preto!
    memset(pic[2].img, 0, width * height * 3);

    // Cria textura para a imagem de saída
    tex[2] = SOIL_create_OGL_texture((unsigned char *)pic[2].img, pic[2].width, pic[2].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

    // Entra no loop de eventos, não retorna
    glutMainLoop();
}

// Gerencia eventos de teclado
void keyboard(unsigned char key, int x, int y)
{
    if (key == 27)
    {
        // ESC: libera memória e finaliza
        freemem();
        exit(1);
    }
    if (key >= '1' && key <= '3')
        // 1-3: seleciona a imagem correspondente (origem, máscara e resultado)
        sel = key - '1';
    if (key == 's')
    {
        seamcarve(targetW);
    }
    glutPostRedisplay();
}

void arrow_keys(int a_keys, int x, int y)
{
    switch (a_keys)
    {
    case GLUT_KEY_RIGHT:
        if (targetW <= pic[2].width - 1)
            targetW += 1;
        seamcarve(targetW);
        break;
    case GLUT_KEY_LEFT:
        if (targetW > 1)
            targetW -= 1;
        seamcarve(targetW);
        break;
    default:
        break;
    }
}
// Faz upload da imagem para a textura,
// de forma a exibi-la na tela
void uploadTexture()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                 target->width, target->height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, target->img);
    glDisable(GL_TEXTURE_2D);
}

// Callback de redesenho da tela
void draw()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Preto
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Para outras cores, veja exemplos em /etc/X11/rgb.txt

    glColor3ub(255, 255, 255); // branco

    // Ativa a textura corresponde à imagem desejada
    glBindTexture(GL_TEXTURE_2D, tex[sel]);
    // E desenha um retângulo que ocupa toda a tela
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);

    glTexCoord2f(0, 0);
    glVertex2f(0, 0);

    glTexCoord2f(1, 0);
    glVertex2f(pic[sel].width, 0);

    glTexCoord2f(1, 1);
    glVertex2f(pic[sel].width, pic[sel].height);

    glTexCoord2f(0, 1);
    glVertex2f(0, pic[sel].height);

    glEnd();
    glDisable(GL_TEXTURE_2D);

    // Exibe a imagem
    glutSwapBuffers();
}
