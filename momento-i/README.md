# Operación Macondo (Momento I: Contexto y Niveles.)

**Temática del 2026-2:** Fantasías animadas de la historia de Colombia  
*(Abarca eventos locales o universales relevantes para Colombia).*
*Evento escogido:* Operación Macondo: Terminator busca a Gabriel García Márquez para entregarle el Nobel

---

## Momento I: Contexto y Niveles.

### Actividades

**A) Extensión del contexto:** En un documento de Word dentro del directorio del momento I, describa (en prosa) la dinámica que plantean para el funcionamiento del juego, inspirada en el guión de la temática elegida.

Plantee al menos dos niveles, cada uno con una dinámica de interacción notablemente diferenciable. Incluya una sinopsis del contexto correspondiente a cada nivel, y para cada nivel: describa detalladamente las dinámicas, interacciones, vistas, físicas a implementar (tres o más), retos, objetivos y funcionamiento general.

---

## Nivel 1

* **Idea:** Terminator inicia su misión en la hermosa ciudad de Cartagena. Montado en su moto[cite: 15], pone marcha hacia Macondo, recorriendo ciudades de la costa hasta llegar a Aracataca (de donde es Gabo) y adentrándose a escenarios inspirados en las obras de Gabriel García Márquez (Macondo). Pero su viaje se ve interrumpido por uno de los personajes de los libros de Gabo: «Un anciano con alas enormes», quien corrompido por Aureliano-Skynet intenta evitar que nuestro protagonista llegue a macondo lanzando botellas mientras nuestro protagonista también tiene que estar atento a los obstáculos del camino, los cuales debe esquivar alternando entre 3 carriles, Este nivel tendrá una  vista con scroll (ilimitado). Objetivo: Sobrevivir al recorrido en motocicleta, esquivando las botellas y los obstáculos durante el trayecto hasta alcanzar el punto de llegada a Macondo. Al lograrlo, Terminator podrá avanzar al segundo nivel.
* **Daño e interacciones:** Terminator tendrá una barra de vida. Las botellas, las totumas caídas, las cascaras de banana le restarán vida; estas últimas también harán que temporalmente (1.5 segundos) Terminator no pueda recibir indicaciones del usuario (“perder control de la moto”) . Chocar contra un obstáculo fijo provocará su muerte inmediata. Si su barra de vida llega a cero, morirá; en estos dos últimos casos el nivel terminará.
* **Física:** Trayectoria parabólica de las botellas — Nivel 1 : El “anciano con alas enormes” lanza botellas hacia la carretera. Estas deben seguir una trayectoria curva y no desplazarse en línea recta. El efecto visible es que la botella describe un arco mientras se acerca a la carretera. Al cambiar la velocidad inicial, el villano principal puede lanzar botellas más lejos o con arcos más pronunciados.
* **Características del personaje:** Mientras Terminator está intentando esquivar los obstáculos podrá ir encontrando premios, los cuales lo harán convertirse en un conductor rapidín, durante unos 4-5 segundos con el objetivo de llegar más rápido al destino.

---

## Nivel 2

* **Idea:** Después de superar el primer nivel, Terminator llega a Macondo, donde Skynet mantiene secuestrado a Gabriel García Márquez para impedir que reciba el Premio Nobel de Literatura. El objetivo principal es rescatar a Gabo y llegar con él a salvo para que pueda recibir el Nobel. Para lograrlo, Terminator debe atravesar un laberinto formado por calles, pasadizos y lugares de Macondo, evitando a los guardias y a Aureliano-Skynet, el villano principal del nivel (un agente autónomo) que lo persigue para capturarlo. El nivel tendrá una vista bidimensional desde arriba (top-down) y un tiempo limitado para completar la misión.
* **Subretos:** Terminator deberá recolectar dos artefactos indispensables para liberar a Gabo: el código de acceso de la puerta, que encontrará en un computador ubicado en una oficina, y un ejemplar de Cien años de soledad, que aparecerá en una ubicación aleatoria del laberinto. Además, podrá recoger una pistola con municiones limitadas para disparar en línea recta (se describe este objeto en características del personaje), con esta puede eliminar guardias pero si le dispara al villano principal, no podrá eliminarlo, pero sí aturdirlo temporalmente, reduciendo su velocidad durante unos segundos.
* **Reto principal:** conseguir ambos artefactos y llegar a la puerta donde está retenido Gabo antes de que se agote el tiempo, evitando ser capturado. Una vez cumplidos los requisitos, Terminator podrá liberarlo y completar la misión.
* **Física:** Movimiento pendular de una trampa del laberinto: en uno los pasillos por donde es obligatorio pasar al inicio del nivel es una cuchilla que está suspendida y oscila de un lado a otro (periódicamente). Terminator debe calcular cuándo cruzar sin ser alcanzado.
* **Movimiento circular:** de una cámara de vigilancia de Skynet ubicada en uno de los pasillos del laberinto, que gira periódicamente sobre un eje fijo y detecta a Terminator cuando entra en su campo de visión. Al detectarlo, activa una alarma que resta 10 segundos al cronómetro del nivel y aumenta la velocidad de Aureliano-Skynet durante 3 segundos, obligando al jugador a buscar rutas alternativas y evitar ser detectado.
* **Características del personaje:** El nivel estará acompañado de un objeto que le será de utilidad a nuestro protagonista (Terminator) el cual será una pistola con la que se convierte en su versión agresivo y podría acabar con alguno de los guardias del laberinto o retrasar al villano del nivel (Aureliano Skynet). nota: dirá su frase célebre “hasta la vista baby” cuando disparé el arma.

---

## Agente autónomo sencillo

* **Hombre con alas grandes:** será el agente autónomo que intenta detener a Terminator mientras conduce su motocicleta por la carretera del Caribe. Se moverá verticalmente para alinearse con el carril del protagonista y lanzará botellas para obstaculizar su avance.
* **Percepción:** detecta en qué carril se encuentra Terminator y calcula su posición vertical.
* **Razonamiento:** decide si debe subir o bajar para alinearse con Terminator y aumentar la probabilidad de acertarle con una botella.
* **Acción:** se desplaza verticalmente y lanza una botella siguiendo una trayectoria parabólica.
* **Aprendizaje:** El villano del nivel (el hombre con alas grandes) recuerda los carriles preferidos de Terminator y el éxito de sus lanzamientos. Esta característica cobra mayor importancia a medida que aumenta la velocidad del juego, pues anticipar los movimientos del protagonista y ajustar el momento de sus ataques le permitirá reaccionar con mayor precisión y representar un desafío creciente para el jugador que tiene menos tiempo para reaccionar.

---

## Aureliano-Skynet (Nivel 2)

* Será el agente autónomo del laberinto. Su objetivo es capturar a Terminator antes de que consiga los dos artefactos y llegue a la puerta donde está retenido Gabriel García Márquez.
* **Percepción:** detecta la posición de Terminator, identifica los pasillos transitables y reconoce si una pared bloquea el camino.
* **Razonamiento:** Teniendo la posición destino (donde está Terminator) y la posición inicial (donde se ubica actualmente en el tablero) calcula una ruta para acercarse a Terminator mediante un algoritmo sencillo de búsqueda de caminos. También puede considerar las rutas que el jugador ha utilizado anteriormente.
* **Acción:** Se desplaza por el laberinto siguiendo la ruta elegida para perseguir a Terminator. Si el jugador le dispara queda aturdido temporalmente y reanuda la persecución pasado el tiempo definido (3 segundos).
* **Aprendizaje:** registra qué rutas utiliza Terminator para escapar y actualiza sus preferencias para perseguirlo por caminos que anteriormente le permitieron acercarse. Así, sus decisiones pueden mejorar durante la partida.
