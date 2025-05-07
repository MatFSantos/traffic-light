# Título do Projeto

Smart Traffic Light

# Objetivo Geral
O objetivo do projeto é controlar um semáforo, tornando ele inteligente e mais seguro para todos os tipos de pedestres e motoristas.

# Descrição Funcional

O projeto foi desenvolvido para controlar um semáforo, a fim de torná-lo inteligente. O controlador, faz a sequência de luz do semáforo habitual, verde por 15 segundos, amarelo por 4 segundos e vermelho por 11 segundos, emitindo sons para que seja possível diferenciar em qual estado o semáforo está, sendo mais inclusivo, principalmente para deficientes visuais que utilizam as ruas da cidade.

Além da funcionalidade básica, o projeto apresenta a opção de mudança de modos do semáforo pelo pressionamento de um botão. O outro modo, além do básico, é o noturno, onde o semáforo fica com a luz amarela, indicando atenção, piscando de forma intermitente. Um som também é emitido, sendo capaz de diferenciar um estado do outro.

O projeto foi feito utilizando RTOS, um sistema operacional, capaz de programar diferentes tarefas para que seja feita o processamento concorrente das mesmas, dando a impressão de execução de todas ao mesmo tempo, fazendo com que o controlador do semáforo faça mais de uma função ao mesmo tempo, sem precisar pausar as outras funções.

# Descreva os pontos mais relevantes tanto do dos Periféricos da BitDogLab/RP2040 quanto do  seu código.

No que diz respeito aos periféricos utilizados, temos a matriz de LEDs RGB 5x5, o display OLED SSD1306, um push button, um buzzer passivo e um LED RGB. 

A matriz de LEDs RGB 5x5 foi utilizada para simular o farol do semáforo, acendendo um quadrado ao centro nas cores verde, amarela e vermelha. A matriz foi atualizada na tarefa principal, que era responsável por trocar as cores do semáforo nos determinados tempos.

Já o display OLED SSD1306 foi usado para apresentar textualmente o estado atual do semáforo, sendo capaz de mostrar com texto se a passagem estava liberada ou não. Uma tarefa única foi criada para controlar tal funcionalidade.

O push button foi usado, também em uma tarefa de exclusividade dele, para fazer a mudança de modos do semáforo, de funcionamento normal ( o habitual) para noturno, e vice-versa.

O  buzzer passivo e o LED RGB foram usados em conjunto, para emitir som e luz de acordo com o estado do semáforo atual, sendo possível distinguir apenas com os beeps apresentados.

Todas as tarefas foram realizadas utilizando o RTOS, um sistema operacional que proporcionou a funcionalidade de executar as tarefas concorrentemente, fazendo com que varias tarefas, e não somente uma como de costume, fosse executada na BitDogLab. Ao total foram desenvolvidas quatro tarefas, que foram instanciadas e iniciadas na função principal.

A primeira tarefa, vTrafficLightTask, foi responsável por acender o semáforo (matriz de LEDs RGB 5x5) no estado correto e também fazer as trocas de estado.

A segunda tarefa, vButtonTask, foi responsável, por meio de pooling, por monitorar o botão A (push button), botão de alteração do modo do semáforo. Caso o botão fosse pressionado, a tarefa faria a alteração do modo.

A terceira tarefa, vBeepTask, fez o controle do buzzer passivo e do LED RGB, fazendo com que o som e a luz fossem emitidos no momento certo dos estados do semáforo.

A quarta e última tarefa, vDisplayTask, foi responsável por atualizar o display OLED SSD1306.

Todas as tarefas tiveram um delay em seus loops de 50 milissegundos, utilizando a função do RTOS vTaskDelay, que pausa a tarefa, mas deixa o RTOS livre para fazer as demais.

Por fim, os requisitos foram cumpridos, fazendo com que o projeto tenha sido concluído em sua totalidade, implementando o semáforo, com tempos distintos entre os estados verde, amarelo e vermelho, implementando o aviso sonoro (e mais um visual). Além disso, o modo noturno também foi implementado, fazendo com que o semáforo pisque de forma intermitente o farol amarelo.

# Link vídeo

[Clique para abrir o vídeo](https://drive.google.com/file/d/1AjWi29Hp8S-UTJsGDqXHFkyyZmeRujaf/view?usp=sharing)