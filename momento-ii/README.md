# Operación Macondo

## Momento II: Análisis y diseño de la capa lógica

### 1. Contexto general del videojuego

*Operación Macondo: Terminator en busca de Gabriel García Márquez* es un videojuego que combina elementos de ciencia ficción con la historia y la literatura colombianas. La propuesta integra al personaje de Terminator con el universo literario de Gabriel García Márquez, tomando como referencia la importancia cultural de su obra y la obtención del Premio Nobel de Literatura en 1982.

La historia comienza cuando Skynet intenta alterar el curso de los acontecimientos para impedir que Gabriel García Márquez reciba el Premio Nobel de Literatura. Para cumplir esta misión, Terminator deberá recorrer escenarios inspirados en la costa Caribe colombiana, llegar a Macondo y enfrentarse a los obstáculos y enemigos enviados por Skynet.

Para consultar la narración y la descripción de los niveles, visita la carpeta [**Momento I — Contexto y Niveles**](../momento-i/).

El objetivo final consiste en rescatar a Gabriel García Márquez y permitir que llegue a salvo para recibir el Premio Nobel.

### 2. Diagrama de clases

El siguiente diagrama representa la estructura de clases del videojuego *Operación Macondo*, incluyendo sus principales clases, atributos, métodos y relaciones.

![Diagrama de clases del videojuego](./imagenes/Desafio2.png)

[Ver documento PDF original](./Desafio2.pdf)

### 2. Niveles.
En esta sección se presentan los sprites correspondientes a cada uno de los niveles del videojuego Operación Macondo: Terminator en busca de Gabriel García Márquez. Para el *nivel 1*, se incluyen los sprites de los dos personajes principales: *Terminator*, controlado por el jugador, y el *Hombre con alas enormes*, que actúa como agente enemigo (villano principal del nivel). Además, se muestran dos fondos que representan los escenarios por los que transcurre el recorrido en motocicleta, *Macondo y Santa Marta*. Para el *nivel 2*, se presentarán los sprites de los personajes, enemigos y objetos necesarios para representar el laberinto de Macondo y las mecánicas de exploración, persecución y rescate de Gabriel García Márquez.

#### 2.1 Nivel 1.
Terminator nivel 1
![Terminator nivel 1](./imagenes/nivel1.png)
El Hombre de alas grandes nivel 1
![El Hombre de alas grandes nivel 1](./imagenes/nivel1.1.png)
Fondo nivel 1(macondo)
![Fondo nivel 1(macondo)](./imagenes/nivel1-fondo.png)
Fondo nivel 1(santamarta)
![Fondo nivel 1(santamarta)](./imagenes/nivel1-fondo-santa-marta.png)

#### 2.2 Nivel 2.
Terminator nivel 2
![Terminator nivel 2](./imagenes/terminator-nivel2.png)
Soldado (personaje secundario) nivel 2
![Soldado (personaje secundario) nivel 2](./imagenes/soldado-nivel2.png)
Aureliano-Skainet nivel 2
![Aureliano-Skainet nivel 2](./imagenes/villano-nivel2.png)
Objetos nivel 2
![Objetos nivel 2](./imagenes/objetos-nivel2.png)

### 2. Diagrama de flujo
En esta sección se presentan los diagramas de flujo correspondientes a los agentes autónomos de cada nivel del videojuego Operación Macondo: Terminator en busca de Gabriel García Márquez. Estos diagramas describen el funcionamiento lógico de cada agente, incluyendo la información que percibe del entorno, las decisiones que toma según las condiciones del juego y las acciones que ejecuta. Además, se explica cómo se comportan los agentes durante la partida y cómo adaptan sus estrategias para interactuar con el jugador, de acuerdo con las mecánicas y los objetivos establecidos para cada nivel.

*Agente nivel 1:*
[Agente nivel 1](./Agente_Nivel1_Anciano_alas_enormes.drawio.pdf)
*Agente nivel 2:*
[Agente nivel 2](./Agente_Nivel1_Agente_Nivel2_Aureliano_Skynet.drawio.pdf)