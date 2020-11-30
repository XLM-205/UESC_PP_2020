
# Binarizador de Imagem
### Grupo:
- Swami de Paula Lima
- Ramon Darwich de Menezes

### Arquivos Fonte:
- ```SourceSingle.c``` executa em somente 1 thread
- ```SourceMulti.c``` executa com partes do código em paralelo
- ```SourceVideoSingle.cpp``` executa em somente 1 thread com funções para binarização em tempo real de videos
	
### Compilação:
	gcc -o O3.out -O3 -fopenmp SourceSingle.c -lm
	gcc -o O3.out -O3 -fopenmp SourceMulti.c -lm
	Compilação para o SourceVideoSingle.cpp pendente
	
### Dependências:
- stb_image.h
- stb_image_write.h
	- De: https://github.com/nothings/stb
- opencv.hpp e suas .dlls
	- De: https://github.com/opencv/opencv

### Entrada:
- Imagem em qualquer formato (.PNG, .BMP, .JPG/.JPEG) OU
- Video em formato .MP4 (Atualmente unico formato testado)

### Saída:
- Imagem binarizada da entrada e um arquivo .txt com o limiar e histograma em escala de cinza. OU
    - Se for dado um nome para a saída, ambos arquivos terão esse nome
    - Se **não** for dado um nome, produzirá ```BINA_<nomeDaImagem>.png``` e ```HIST_<nomeDaImagem>.txt```
- Video binarizado em tempo real (Arquivo de video binarizado futuramente)

### Formatação do Arquivo de Histograma
- 258 linhas de texto, seguindo a ordem:
    -  <Inteiro - Limiar calculado [0..255]>\n
    -  <Inteiro - Quantidade de pixeis que tenham escala de cinza 0>\n
    -  <Inteiro - Quantidade de pixeis que tenham escala de cinza 1 (Repete até EOF, total de 256 linhas)>\n

### Argumentos ('<>' são obrigatórios e '()' opcionais):
- <imagemDeEntrada.formato>
    - Produz ```BINA_<nomeDaImagem>.png``` e ```HIST_<nomeDaImagem>.txt```
- <imagemDeEntrada.formato> (nomeDeSaida.formato)
    - Produz ```<nomeDeSaida>.png``` e ```<nomeDeSaida>.txt```
- <imagemDeEntrada.formato> ([modificadores])
    - Produz ```BINA_<nomeDaImagem>.png``` e ```HIST_<nomeDaImagem>.txt```, com modificadores.
- <imagemDeEntrada.formato> (nomeDeSaida.formato) ([modificadores])
    - Produz ```<nomeDeSaida>.png``` e ```<nomeDeSaida>.txt```, com modificadores.

### Modificadores:
Todos os modificadores são especificados com '-' ou '/', uma letra-chave, maiúscula ou minuscula, e, a depender do modificador, um número. Podem ser usados uma unica vez, em qualquer ordem após a imagem de entrada.
- -i
	- Trata a entrada como sendo uma imagem. Por padrão, a entrada será tratada como um video
- -v
    - Executa o programa em modo 'verbose', mostrando detalhes da imagem, limiar e consumo de tempo (mais lento).
- -t <[0..255]>	
    - Limiar inicial. Se não estiver presente, usa-se o valor padrão (127).
- -e <[0..255]>	
    - Margem de erro para o calculo do limiar. Define quantas escalas de cinza o limiar pode ter de erro para ser aceito. Se não estiver presente, usa-se o valor padrão (5)
- -r
    - Define o calculo de erro para **'relativo'**. Se não estiver presente, usa-se o modo padrão ('absoluto').
