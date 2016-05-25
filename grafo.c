#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <graphviz/cgraph.h>
#include "grafo.h"
#include "lista.h"
 
// tamanho máximo da string para representar o peso de uma aresta/arco
#define MAX_STRING_SIZE 256

//------------------------------------------------------------------------------
// (apontador para) estrutura de dados para representar um grafo
//
// o grafo pode ser
// - direcionado ou não
// - com pesos nas arestas ou não
//
// além dos vértices e arestas, o grafo tem um nome, que é uma "string"
//
// num grafo com pesos nas arestas todas as arestas tem peso, que é um long int
//
// o peso default de uma aresta é 0
struct grafo{
    char *nome; //nome do grafo
    int direcionado; // 1, se o grafo é direcionado, 0 se não é
    int ponderado; // 1, se o grafo tem pesos nas arestas/arcos, 0 se não é
    unsigned int n_vertices; //numero de vertices
    unsigned int n_arestas; //numero de arestas
    vertice *vertices; //apontador para a estrutura de vertices
};
//------------------------------------------------------------------------------
// (apontador para) estrutura de dados que representa um vértice do grafo
//
// cada vértice tem um nome que é uma "string"
struct vertice{
    char *nome; // nome do vertice
    unsigned int id; // id = posição do vertice no vetor de vertices do grafo, serve para facilitar a busca de vertices
    unsigned int grau_entrada; // grau do vertice
    unsigned int grau_saida; // grau do vertice
    int removido; // se for 1 a aresta do grafo foi removida, se for 0, nao
    int tamRotulo;    
    int passado;
    unsigned long long int rotulo;
    //lista rotulos; //rotulo do vertice {1..n}
    lista adjacencias_entrada;
    lista adjacencias_saida;
};
//------------------------------------------------------------------------------
// estrutura dos vizinhos
typedef struct adjacencia{
    long int peso;
    vertice v_origem; //vertice de origem
    vertice v_destino; //vertice de destino
} *adjacencia;
//------------------------------------------------------------------------------
// cria e devolve um  grafo g
 
static grafo cria_grafo(const char *nome, int direcionado, int ponderado, int total_vertices){
    grafo g = malloc(sizeof(struct grafo)); //aloca memoria pro grafo
 
    if(g == NULL)
    return 0;
   
    g->nome = malloc((strlen(nome) + 1) * sizeof(char));
    strcpy(g->nome, nome);
    g->direcionado = direcionado;
    g->ponderado = ponderado;
    g->n_vertices = 0;
    g->n_arestas = 0;
    g->vertices = malloc((unsigned int)total_vertices* sizeof(vertice));
 
    return g;
    free(g->nome);
    free(g);
}
 
//------------------------------------------------------------------------------
// cria um vizinho e insere na lista de vizinhos do vertice de origem e/ou na de destino tambem
// se ele nao for direcionado
 
static void cria_vizinhanca(grafo g, vertice origem, vertice destino, long int peso){
    adjacencia viz_1 = malloc(sizeof(struct adjacencia));
   
    if(viz_1 == NULL)
    printf("Sem memoria");
    else{
        viz_1->peso = peso;
        viz_1->v_origem = origem;
        viz_1->v_destino = destino;
		insere_lista(viz_1, origem->adjacencias_saida);
        origem->grau_saida++;
        destino->grau_entrada++;
 
        if (!direcionado(g)) {
            // se o grafo não for direcionado, a aresta deve aparecer também na
            // lista de adjacencia do vertice dest
 
            adjacencia viz_2 = malloc(sizeof(struct adjacencia));
            if(viz_2 == NULL)
            printf("Sem memoria");
            else{
                viz_2->peso = peso;
                viz_2->v_origem = destino;
                viz_2->v_destino = origem;
                insere_lista(viz_2, destino->adjacencias_saida);
                destino->grau_saida++;
                origem->grau_entrada++;
                }
        }
    else{
        adjacencia viz_3 = malloc(sizeof(struct adjacencia));
            if(viz_3 == NULL)
            printf("Sem memoria");
            else{
                viz_3->peso = peso;
                viz_3->v_origem = origem;
                viz_3->v_destino = destino;
                insere_lista(viz_3, destino->adjacencias_entrada);
                }
        }
    }
    g->n_arestas++;
}
 
//------------------------------------------------------------------------------
// destroi um vizinho
 
static int destroi_vizinho(adjacencia v){
    free(v);
    return 1;
}
 
//------------------------------------------------------------------------------
// cria, insere no grafo e retorna o vertice
static vertice cria_vertice(grafo g, const char *nome){
    vertice v = malloc(sizeof(struct vertice));
 
    if(v == NULL){
        return 0;
    }else{
        v->id = g->n_vertices;
        v->nome = malloc((strlen(nome) +1) *sizeof(char));
        strcpy(v->nome, nome);
        v->adjacencias_saida = constroi_lista();
        v->adjacencias_entrada = constroi_lista();
        v->grau_entrada = 0;
        v->grau_saida = 0;
        v->removido = 0;
        v->rotulo = 0;
        g->vertices[v->id] = v;
        g->n_vertices++;

    }
 
    return v;
    free(v->nome);
    free(v);
}
 
//------------------------------------------------------------------------------
// destroi um vertice
static void destroi_vertice(vertice v){
    destroi_lista(v->adjacencias_saida, (int (*)(void *)) destroi_vizinho);
    destroi_lista(v->adjacencias_entrada, (int (*)(void *)) destroi_vizinho);
    v->grau_saida = 0;
    v->grau_entrada = 0;
    free(v->nome);
    free(v);
    v =  NULL;
}
 
//------------------------------------------------------------------------------
// busca um vertice pelo nome do grafo
 
static vertice v_busca(grafo g, char *nome){
    for(unsigned int i = 0; i < g->n_vertices; i++){
        if(strcmp(g->vertices[i]->nome, nome) == 0)
            return g->vertices[i];
    }
    return NULL;
}
 
//------------------------------------------------------------------------------
// devolve o peso de uma aresta no formato libcgraph
 
static long int get_peso(Agedge_t *Ae) {
    char p_str[5];
    strcpy(p_str, "peso");
    char *peso_c = agget(Ae, p_str);
 
    if (peso_c && *peso_c)
        return atol(peso_c);
 
    return 0;
}
 
//------------------------------------------------------------------------------
// devolve 1 se o grafo no formato libcgraph tem pesos nas arestas
 
static int contem_pesos(Agraph_t *Ag) {
    char p_str[5];
    strcpy(p_str, "peso");
 
    for ( Agsym_t *sym = agnxtattr(Ag, AGEDGE, NULL);
        sym;
        sym = agnxtattr(Ag, AGEDGE, sym) ) {
        if (strcmp(sym->name, p_str) == 0)
            return 1;
    }
 
    return 0;
}
 
//------------------------------------------------------------------------------
// devolve o nome do grafo g
 
char *nome_grafo(grafo g){
    char nome_g[1];
    nome_g[0] = '\0';
    return g ? g->nome : nome_g;
}
 
//------------------------------------------------------------------------------
// devolve 1, se g é direcionado, ou
//         0, caso contrário
 
int direcionado(grafo g){
    return g ? g->direcionado : 0;
}
 
//------------------------------------------------------------------------------
// devolve 1, se g tem pesos nas arestas/arcos,
//      ou 0, caso contrário
 
int ponderado(grafo g){
    return g ? g->ponderado : 0;
}
 
//------------------------------------------------------------------------------
// devolve o número de vértices do grafo g
 
unsigned int n_vertices(grafo g){
    return g ? g->n_vertices : 0;
}
 
//------------------------------------------------------------------------------
// devolve o número de arestas/arcos do grafo g
 
unsigned int n_arestas(grafo g){
    return g ? g->n_arestas : 0;
}
 
//------------------------------------------------------------------------------
// devolve o nome do vertice v
 
char *nome_vertice(vertice v){
    char nome_v[1];
    nome_v[0] = '\0';
    return v ? v->nome : nome_v;
}
 
//------------------------------------------------------------------------------
// lê um grafo no formato dot de input, usando as rotinas de libcgraph
//
// desconsidera todos os atributos do grafo lido exceto o atributo
// "peso" quando ocorrer; neste caso o valor do atributo é o peso da
// aresta/arco que é um long int
//
// num grafo com pesos todas as arestas/arcos tem peso
//
// o peso default de uma aresta num grafo com pesos é 0
//
// todas as estruturas de dados alocadas pela libcgraph são
// desalocadas ao final da execução
//
// devolve o grafo lido ou
//         NULL em caso de erro
 
grafo le_grafo(FILE *input){
    if (!input)
        return NULL;
   
    Agraph_t *Ag = agread(input, NULL);
   
    if(!Ag)
        return NULL;
 
    grafo g = cria_grafo(agnameof(Ag), agisdirected(Ag), contem_pesos(Ag), agnnodes(Ag));
 
    for (Agnode_t *Av=agfstnode(Ag); Av; Av=agnxtnode(Ag,Av)) {
            cria_vertice(g, agnameof(Av));
    }
 
        for (Agnode_t *Av=agfstnode(Ag); Av; Av=agnxtnode(Ag,Av)) {
            for (Agedge_t *Ae=agfstout(Ag,Av); Ae; Ae=agnxtout(Ag,Ae)) {
                vertice u = v_busca(g, agnameof(agtail(Ae)));
                vertice v = v_busca(g, agnameof(aghead(Ae)));
                cria_vizinhanca(g, u, v, get_peso(Ae));
            }
        }
   
    agclose(Ag);
    agfree(Ag, NULL);
    return g;
}  
 
//------------------------------------------------------------------------------
// desaloca toda a memória usada em *g
//
// devolve 1 em caso de sucesso ou
//         0 caso contrário
//
// g é um (void *) para que destroi_grafo() possa ser usada como argumento de
// destroi_lista()
 
int destroi_grafo(void *g){
    if (!g)
		return 0;
 
    for (unsigned int i=0; i<((grafo) g)->n_vertices; i++)
        destroi_vertice(((grafo) g)->vertices[i]);
   
    free(((grafo) g)->vertices);
    free(((grafo) g)->nome);
    free(g);
 
    return 1;
}
 
//------------------------------------------------------------------------------
// escreve o grafo g em output usando o formato dot, de forma que
//
// 1. todos os vértices são escritos antes de todas as arestas/arcos
//
// 2. se uma aresta tem peso, este deve ser escrito como um atributo
//    de nome "peso"
//
// devolve o grafo escrito ou
//         NULL em caso de erro
 
grafo escreve_grafo(FILE *output, grafo g){
    if(!g || !output)
        return NULL;
 
    Agraph_t *ag;
    Agsym_t *peso;
 
    char peso_s[MAX_STRING_SIZE];
 
    //criando a string "peso"
    char p_str[5];
    strcpy(p_str, "peso");
 
    //cria uma string vazia pra usar como valor default do atributo peso
 
    char default_s[1];
    default_s[0] = '\0';
 
    if(g->direcionado)
        ag = agopen(g->nome, Agstrictdirected, NULL);
    else
        ag= agopen(g->nome, Agstrictundirected, NULL);
 
    if(g->ponderado)
        peso = agattr(ag, AGEDGE, p_str, default_s);
 
    Agnode_t **nodes = malloc(g->n_vertices * sizeof(Agnode_t*));
   
    for(unsigned int i = 0; i < g->n_vertices; i++)
        nodes[g->vertices[i]->id] = agnode(ag, g->vertices[i]->nome, TRUE);
 
    for(unsigned int i = 0; i < g->n_vertices; i++){
        vertice v = g->vertices[i];
 
        for(no n = primeiro_no(v->adjacencias_saida); n != NULL; n = proximo_no(n)){
            adjacencia viz = conteudo(n);
 
            Agedge_t *ae = agedge(ag, nodes[v->id], nodes[viz->v_destino->id], NULL, TRUE);
               
            if(g->ponderado){
                sprintf(peso_s, "%ld", viz->peso);
                agxset(ae, peso, peso_s);
            }
        }
    }
 
    free(nodes);
    agwrite(ag, output);
    agclose(ag);
    agfree(ag, NULL);
    return g;
}
 
//------------------------------------------------------------------------------
// devolve um grafo igual a g
 
grafo copia_grafo(grafo g){
    if(!g)
        return NULL;
   
    grafo copy = cria_grafo(g->nome, g->direcionado, g->ponderado, (int)g->n_vertices);
 
        if(copy == NULL){
            printf("Sem memoria para alocar.\n");
        }else{
        copy->n_vertices = g->n_vertices;
        copy->n_arestas = g->n_arestas;
 
        for(unsigned int i = 0; i < g->n_vertices; i++){
            copy->vertices[i] = g->vertices[i];
        }
    }
    return copy;
    free(copy);
}
 
//------------------------------------------------------------------------------
// devolve a vizinhança de entrada do vértice v
 
static lista vizinhanca_entrada(vertice v){
    lista viz = constroi_lista();
    for (no n=primeiro_no(v->adjacencias_entrada); n!=NULL; n=proximo_no(n)) {
        adjacencia a = conteudo(n);
        insere_lista(a->v_origem, viz);
    }        
    return viz;
}
 
//------------------------------------------------------------------------------
// devolve a vizinhança de saída do vértice v
 
static lista vizinhanca_saida(vertice v){
 
    lista viz = constroi_lista();
    for (no n=primeiro_no(v->adjacencias_saida); n!=NULL; n=proximo_no(n)) {
        if(n != NULL){
            adjacencia a = conteudo(n);
            insere_lista(a->v_destino, viz);
        }
    }        
    return viz;
}
 
//------------------------------------------------------------------------------
// devolve a vizinhança do vértice v
//
// se direcao == 0, v é um vértice de um grafo não direcionado
//                  e a função devolve sua vizinhanca
//
// se direcao == -1, v é um vértice de um grafo direcionado e a função
//                   devolve sua vizinhanca de entrada
//
// se direcao == 1, v é um vértice de um grafo direcionado e a função
//                  devolve sua vizinhanca de saída
 
lista vizinhanca(vertice v, int direcao, grafo g){
    if (!g)
        return NULL;
   
    if(direcao==-1)
        return vizinhanca_entrada(v);
    else if (direcao == 1 || direcao == 0)
        return vizinhanca_saida(v);
       
    return NULL;
}
 
//------------------------------------------------------------------------------
// devolve o grau do vértice v
// i
// se direcao == 0, v é um vértice de um grafo não direcionado
//                  e a função devolve seu grau
//
// se direcao == -1, v é um vértice de um grafo direcionado
//                   e a função devolve seu grau de entrada
//
// se direcao == 1, v é um vértice de um grafo direcionado
//                  e a função devolve seu grau de saída
 
unsigned int grau(vertice v, int direcao, grafo g){
    if (!g)
        return 0;
    if(direcao < 0)
        return v->grau_entrada;
    else
        return v->grau_saida;
}

//------------------------------------------------------------------------------
static unsigned int findRemoved(lista l){
    unsigned int removed = 0 ;
    for (no n=primeiro_no(l); n!=NULL; n=proximo_no(n)) {
        vertice v = conteudo(n);
        if(v->removido == 1){
           removed ++;
        }
    }
    return removed;
}
 
//------------------------------------------------------------------------------
// devolve 1, se o conjunto dos vertices em l é uma clique em g, ou
//         0, caso contrário
//
// um conjunto C de vértices de um grafo é uma clique em g
// se todo vértice em C é vizinho de todos os outros vértices de C em g
 
    // faz um loop em cima de cada elemento da lista l      
    // para cada elemento pega sua vizinhaca
    // se a vizinhaca conter todos os elementos da lista l continua
    // se não conter retorna 0
   
int clique(lista l, grafo g){
    unsigned int removed = findRemoved(l);
    for (no n=primeiro_no(l); n!=NULL; n=proximo_no(n)) {
        vertice v = conteudo(n);
        if(v->removido == 0){  
            lista vizinhos = vizinhanca(v,0,g);
            unsigned int todos_nos = tamanho_lista(l) - removed - 1;
            for (no auxN=primeiro_no(vizinhos); auxN!=NULL; auxN=proximo_no(auxN)) {
                if(todos_nos == 0)
                    break;
                vertice auxV = conteudo(auxN);
                if(auxV->removido == 0){
                    for (no verifiyNode=primeiro_no(l); verifiyNode!=NULL; verifiyNode=proximo_no(verifiyNode)) {
                        vertice verifyVertice = conteudo(verifiyNode);  
                        if(verifyVertice->removido == 0){
                            if(strcmp(verifyVertice->nome,auxV->nome) == 0){
                                todos_nos--;
                                break;
                            }  
                        }
                    }
                }
            }
            if(todos_nos != 0){
                return 0;
            }
        }
    }  
    return 1;
}
 
//------------------------------------------------------------------------------
// devolve 1, se v é um vértice simplicial em g, ou
//         0, caso contrário
//
// um vértice é simplicial no grafo se sua vizinhança é uma clique
 
int simplicial(vertice v, grafo g){      
    lista vizinhos = vizinhanca(v,0,g);    
    return clique(vizinhos,g);
}



static void generateNumberRotule(vertice v, unsigned int rotulo){
    
    if(v->rotulo == 0){
        v->rotulo = rotulo;
    }
    else{
        unsigned long long int length =(unsigned long long int) snprintf(NULL, 0,"%d",v->rotulo);
        v->rotulo = v->rotulo * (potencia(10,length)) + rotulo;
    }
    
}

// Retorna o vertice lexico
static vertice findLexico(grafo g){
        unsigned long long int maior = 0;
        unsigned int indexLexico = g->n_vertices+1;

        for(unsigned int i=0; i < g->n_vertices; i++){
            if(g->vertices[i]->passado!=1 && g->vertices[i]->rotulo!=NULL){
                if(g->vertices[i]->rotulo > maior){
                    maior = g->vertices[i]->rotulo;
                    indexLexico = i;
                }
            }
        }

        if(indexLexico < g->n_vertices+1)
            return g->vertices[indexLexico];
        else
            return NULL;
}

static unsigned long long int potencia(unsigned int x,  unsigned long long int y){
    return (unsigned long long int) pow((double)x,(double)y);
}


static void rotulaVizinhaca(vertice raiz, grafo g){
    lista vizinhos = vizinhanca(raiz,0,g);    
    for (no n=primeiro_no(vizinhos); n!=NULL; n=proximo_no(n)) {
        unsigned long long int length =(unsigned long long int) snprintf(NULL, 0,"%d",v->rotulo);
        unsigned long long int rotuloRaiz =(unsigned int) raiz->rotulo / (potencia(10,length-1));

        printf("Rotulo raiz_rotulaVizinhaca: %d\n", rotuloRaiz);
        vertice auxV = conteudo(n);
        generateNumberRotule(auxV,rotuloRaiz-1);
    }  
}

//------------------------------------------------------------------------------
// devolve uma lista de vertices com a ordem dos vértices dada por uma 
// busca em largura lexicográfica

lista busca_largura_lexicografica(grafo g){
    lista arvore = constroi_lista();
    int naoTerminou=1;
    if(g->vertices!=NULL){
        unsigned int rotulo = g->n_vertices;
        vertice raiz = g->vertices[0];
        generateConcatRotule(raiz,rotulo);
        rotulaVizinhaca(raiz,g);

        raiz->passado = 1;
        insere_lista(raiz, arvore);
        while(naoTerminou){
            vertice lexico = findLexico(g);
            if(lexico!=NULL){
                rotulaVizinhaca(lexico,g);
                insere_lista(lexico, arvore);
            }
            else{
                naoTerminou = 0;
            }
        }
    }

    return arvore;
}


//------------------------------------------------------------------------------
// devolve 1, se a lista l representa uma 
//            ordem perfeita de eliminação para o grafo g ou
//         0, caso contrário
//
// o tempo de execução é O(|V(G)|+|E(G)|)

int ordem_perfeita_eliminacao(lista l, grafo g){
    return 0;
}

//------------------------------------------------------------------------------
// devolve 1, se g é um grafo cordal ou
//         0, caso contrário

int cordal(grafo g){
	grafo copy = copia_grafo(g);
    lista lexica = busca_largura_lexicografica(copy);
    return ordem_perfeita_eliminacao(lexica,copy);
}


