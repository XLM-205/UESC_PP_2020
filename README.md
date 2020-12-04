# Binarizador de Imagem
### Grupo:
- Swami de Paula Lima
- Ramon Darwich de Menezes

### Arquivo Fonte:
- ```Source.cpp``` executa em paralelo com funções para binarização em tempo real e escrita de videos e imagens.
	
### Compilação dos Prototipos SourceSingle.c e SourceMulti.c:
	gcc -o O3.out -O3 -fopenmp SourceSingle.c -lm
	
### Compilação do Source.cpp:
	Execute o arquivo bash "compileVideo.sh"

### Dependências:
- omp.h
	- De: https://www.openmp.org/resources/openmp-compilers-tools/
- stb_image.h
- stb_image_write.h
	- De: https://github.com/nothings/stb
- opencv.hpp e suas .dlls
	- De: https://github.com/opencv/opencv
	
	A configuração do openCV pode ser trabalhosa e tediosa. 
	Por conta disso, há um arquivo para facilitar. Execute no bash o Shell Script: openCVSetup.sh
	Note que ele irá baixar e compilar o openCV. Isso pode levar algum tempo.

### Entrada:
- Imagem ou Video em qualquer formato.

### Saída:
- Video binarizado:
	- Se no modo 'verbose', em tempo real.
	- Se for dado um nome para a saída, um arquivo .avi com esse nome.
	- Se **não** for dado um nome, produzirá ```BINA_<nomeDoVideo>.avi```.
- Imagem binarizada da entrada e um arquivo .txt com o limiar e histograma em escala de cinza. OU
    - Se for dado um nome para a saída, ambos arquivos terão esse nome.
    - Se **não** for dado um nome, produzirá ```BINA_<nomeDaImagem>.png``` e ```HIST_<nomeDaImagem>.txt```.

### Formatação do Arquivo de Histograma para Imagens
- 258 linhas de texto, seguindo a ordem:
    -  <Inteiro - Limiar calculado [0..255]>\n
    -  <Inteiro - Quantidade de pixeis que tenham escala de cinza 0>\n
    -  <Inteiro - Quantidade de pixeis que tenham escala de cinza 1 (Repete até EOF, total de 256 linhas)>\n

### Argumentos ('<>' são obrigatórios e '()' opcionais):
- <videoDeEntrada.formato>
    - Produz ```BINA_<nomeDoVideo>.avi```
- <videoDeEntrada.formato> (nomeDeSaida.formato)
    - Produz ```<nomeDoVideo>.avi```
- <videoDeEntrada.formato> ([modificadores])
    - Produz ```BINA_<nomeDoVideo>.avi```, com modificadores.
- <videoDeEntrada.formato> (nomeDeSaida.formato) ([modificadores])
    - Produz ```<nomeDeSaida>.avi```, com modificadores.

### Modificadores:
Todos os modificadores são especificados com '-' ou '/', uma letra-chave, maiúscula ou minuscula, e, a depender do modificador, um número. Podem ser usados uma unica vez, em qualquer ordem após a imagem de entrada.
- -H
	- Mostra os modificadores disponiveis. Se usado, o programa será encerrado. **Precisa ser chamado como sendo o unico argumento**. Se usado depois, será ignorado.
- -I
	- Trata a entrada como sendo uma imagem. Por padrão, a entrada será tratada como um video.
- -V
    - Executa o programa em modo 'verbose', mostrando detalhes da imagem ou video, limiar e consumo de tempo (mais lento).
	- **Nota: O modo 'verbose' para videos NÃO produz arquivo de saida e roda em uma unica thread.**
- -L <[0..255]>	
    - Limiar inicial. Se não estiver presente, usa-se o valor padrão (127).
- -T <[1..Max do Sistema]>
	- Define quantas threads o programa vai usar. Se for definido um numero maior que o maximo do sistema, ele será alterado para o maximo disponivel. Se não estiver presente, usa-se o valor padrão (Maximo disponivel do Sistema).
- -E <[0..255]>	
    - Margem de erro para o calculo do limiar. Define quantas escalas de cinza o limiar pode ter de erro para ser aceito. Se não estiver presente, usa-se o valor padrão (5).
- -Q <[1..X]>
	- Define o tamanho da fila de leitura. Valores altos que sejam multiplos da quantidade de threads disponiveis se mostraram mais eficazes. Se não estiver presente, usa-se o valor padrão (120).
- -R
    - Define o calculo de erro para **'relativo'**. Se não estiver presente, usa-se o modo padrão ('absoluto').
    
    
### Exemplos de uso:

	Video videoteste.mp4		Mostra uma janela em tempo real do video sendo binarizado.
	Video -i teste.png			Cria uma imagem binarizada a partir de teste.png.
	Video -h					Mostra os modificadores disponiveis e seus limites
