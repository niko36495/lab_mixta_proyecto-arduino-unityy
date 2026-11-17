# LabMixta — Gemelo digital de un velocímetro de vehículo

Diseño e implementación de un **gemelo digital** de un velocímetro: un sistema físico (Arduino + potenciómetro + servomotor) sincronizado en ambos sentidos con una escena 3D en **Unity**. Al mover el potenciómetro, o el slider digital en pantalla, cambian a la vez la aguja del velocímetro, el texto en km/h, el giro de las ruedas, el movimiento de la carretera y el ángulo del servo.
---

## Tabla de contenidos

1. [¿Qué hace?](#qué-hace)
2. [Arquitectura](#arquitectura)
3. [Hardware](#hardware)
4. [Protocolo serial](#protocolo-serial)
5. [Política de arbitraje](#política-de-arbitraje)
6. [Estructura del proyecto](#estructura-del-proyecto)
7. [Instalación y puesta en marcha](#instalación-y-puesta-en-marcha)
8. [Configuración de los scripts](#configuración-de-los-scripts)
9. [Resultados experimentales](#resultados-experimentales)
10. [Solución de problemas](#solución-de-problemas)
11. [Créditos y licencias](#créditos-y-licencias)

---

## ¿Qué hace?

- **Dos fuentes de control** para una misma variable (la velocidad, 0–180 km/h):
  - **POT**: potenciómetro físico conectado al Arduino.
  - **DIG**: slider digital en la interfaz de Unity.
- **Gemelo digital en Unity**: una camioneta recorre una carretera infinita rodeada de bosque, con un velocímetro (aguja + texto) que refleja la velocidad en tiempo real.
- **Gemelo físico**: un servomotor reproduce el ángulo (0°–180°) equivalente a la velocidad actual, con un transportador de papel como referencia visual.
- **Sincronización bidireccional**: cualquiera de las dos fuentes actualiza a la otra, sin bucles de realimentación.

## Arquitectura

```
┌────────────────────────┐   USB / Serial 9600 baud   ┌─────────────────────────┐
│        ARDUINO UNO     │ ◄────────────────────────► │          UNITY          │
│                        │                            │                         │
│  Potenciómetro (POT)   │ ── "S:<ángulo>,POT\n" ───► │  ArduinoSerialController│
│  Servomotor            │ ◄── "A:<ángulo>\n" ─────── │        │ (slider)       │
│                        │ ── "S:<ángulo>,DIG\n" ───► │        ▼                │
└────────────────────────┘                            │  ControlVelocidad       │
                                                      │   ├─ Aguja + texto km/h │
                                                      │   ├─ Giro de ruedas     │
                                                      │   └─ MovimientoCarretera│
                                                      └─────────────────────────┘
```

**Flujo de datos**

1. Unity abre el puerto serial en un **hilo secundario** que lee líneas y las encola en una `ConcurrentQueue`; el hilo principal las procesa en `Update()` (así la lectura nunca bloquea el render).
2. Si el usuario mueve el **slider digital**, Unity envía `A:<ángulo>` al Arduino para mover el servo.
3. Si el Arduino reporta un cambio (`S:<ángulo>,POT` o `S:<ángulo>,DIG`), Unity actualiza el slider con `SetValueWithoutNotify`, evitando reenviar el valor de vuelta (sin eco ni bucles).
4. `ControlVelocidad` lee el slider cada frame y deriva todo lo visual a partir de un único porcentaje de velocidad.

## Hardware

Según el montaje mostrado en el video:

| Componente | Función |
|---|---|
| Arduino UNO | Microcontrolador, comunicación serial con Unity |
| Potenciómetro | Fuente de control física (POT) |
| Micro servomotor (tipo SG90) | Representa físicamente el ángulo/velocidad |
| Protoboard + cables | Conexiones |
| Transportador impreso/dibujado | Referencia del ángulo del servo (0°–180°) |
| Cable USB | Alimentación y comunicación con el PC |

## Protocolo serial

Configuración: **9600 baudios**, terminador de línea `\n`. Los valores son enteros entre **0 y 180**.

| Dirección | Mensaje | Significado |
|---|---|---|
| Unity → Arduino | `A:<ángulo>` | Mover el servo al ángulo indicado (se envía al mover el slider digital) |
| Arduino → Unity | `S:<ángulo>,POT` | El potenciómetro fijó este ángulo |
| Arduino → Unity | `S:<ángulo>,DIG` | El Arduino confirma un ángulo originado por la fuente digital |

Ejemplos: `A:90` · `S:135,POT` · `S:42,DIG`

Los mensajes con formato incorrecto o que no empiecen por `S:` se ignoran (solo se registran en la consola de Unity).

## Política de arbitraje

El sistema tiene **dos fuentes escribiendo sobre la misma variable**, por lo que necesita una regla para decidir quién manda en cada momento. En esta implementación:

- Existe **un único valor de velocidad** (el slider, 0–180) que alimenta toda la escena.
- Cualquier fuente puede tomar el control: el mensaje `S:<ángulo>,<origen>` se acepta si el origen es `POT` o `DIG`.
- Las actualizaciones que llegan **desde** el Arduino no vuelven a enviarse hacia él (`SetValueWithoutNotify`), de modo que no se producen oscilaciones entre ambas fuentes.

## Estructura del proyecto

```
Assets/
├── ArduinoSerialController.cs   ← Comunicación serial (hilo de lectura + cola de mensajes)
├── ControlVelocidad.cs          ← Slider → aguja, texto km/h, ruedas y carretera
├── MovimientoCarretera.cs       ← Scroll infinito de los tramos de carretera
├── Scenes/
│   └── SampleScene.unity        ← Escena principal del proyecto
├── InputSystem_Actions.inputactions
├── Settings/                    ← Perfiles URP (PC y Mobile)
├── Cielo.mat · lineas.mat · New Material.mat · New Terrain.asset
└── (assets de terceros — ver Créditos)
    ├── Designersoup Low Poly Car Pack Volume 1 updated/
    ├── KayKit_Forest_Nature_Pack_1.0_FREE/
    ├── Props_Tools_Fencing_Free/
    ├── In Game UI Assets/
    ├── rust-wheel/ · pack_cartoon_cars/
    ├── Grass001_2K-JPG (1)/ · asphalt_pit_lane_4k.blend/
    ├── TextMesh Pro/
    └── kloofendal_48d_partly_cloudy_puresky_4k.exr   ← HDRI del cielo
```

### Jerarquía de `SampleScene`

| GameObject | Contenido / script |
|---|---|
| `ComunicacionArduino` | `ArduinoSerialController` |
| `ControlVelocidad` | `ControlVelocidad` |
| `ControlCarretera` | `MovimientoCarretera` (con `Tramo1` … `Tramo5`) |
| `Canvas` | `SliderVelocidad`, `Velocimetro` (con `Pivote` → `aguja`) y texto TMP «km/h» |
| Vehículo | Camioneta *Toyoyo Highlight* con ruedas como `Transform` rotables |
| `Piso`, `Directional Light`, `Global Volume`, `Main Camera`, `EventSystem` | Entorno e iluminación |

### Scripts

**`ArduinoSerialController`** — gestiona el puerto serial.
Parámetros: `puerto` (por defecto `COM3`), `velocidadPuerto` (`9600`), `sliderVelocidad`.

**`ControlVelocidad`** — convierte el valor del slider (0–180) en un porcentaje y lo aplica a:
- el texto (`"<n> km/h"`),
- la rotación de la aguja (de `anguloMinimo = 140°` a `anguloMaximo = -140°`),
- el giro de cada rueda (hasta `giroMaximo = 720°/s` sobre `ejeDeRotacion`),
- la velocidad del fondo (`velocidadMaximaFondo = 30`).

**`MovimientoCarretera`** — desplaza los `tramos` hacia atrás (eje −Z) a la `velocidad` indicada; cuando el tramo trasero cruza `limiteSalida` (−30), lo reubica delante del último tramo (`separacion = 50`), creando una carretera infinita.

## Instalación y puesta en marcha

### Requisitos

- **Unity** con **Universal Render Pipeline (URP)** (el proyecto incluye perfiles URP). Se recomienda Unity 6 o una versión LTS reciente.
- Paquetes: **Input System** y **TextMeshPro**.
- **Arduino IDE** para cargar el sketch.
- Sistema operativo con soporte de puertos seriales (probado con puerto `COMx` de Windows).

### Pasos

1. **Crear un proyecto nuevo** en Unity con la plantilla **Universal 3D (URP)**.
2. Importar el paquete: *Assets → Import Package → Custom Package…* y elegir `LabMixta.unitypackage` (≈ 400 MB, la importación puede tardar).
3. **Habilitar `System.IO.Ports`** *(paso clave)*:
   *Edit → Project Settings → Player → Other Settings → Api Compatibility Level* → **`.NET Framework`** (en Unity 6: *.NET Framework*, no *.NET Standard*).
4. Abrir `Assets/Scenes/SampleScene.unity`.
5. **Cargar el sketch en el Arduino** y anotar el puerto (en el IDE: *Herramientas → Puerto*).
6. Seleccionar `ComunicacionArduino` y poner en `Puerto` el mismo valor (ej. `COM3`).
7. **Cerrar el Monitor Serie** del Arduino IDE (si está abierto, ocupa el puerto).
8. Pulsar **Play** en Unity. En la consola debe aparecer `Arduino conectado en COMx`.

### Probarlo

- Gira el **potenciómetro** → el slider, la aguja, las ruedas y la carretera responden, y el servo se mueve.
- Arrastra el **slider** en pantalla → el servo se mueve al ángulo correspondiente.


## Configuración de los scripts

Si importas el paquete en un proyecto nuevo, comprueba que estas referencias estén asignadas en el Inspector:

| Componente | Campo | Debe apuntar a |
|---|---|---|
| `ArduinoSerialController` | `Slider Velocidad` | `SliderVelocidad` |
| `ControlVelocidad` | `Slider Velocidad` | `SliderVelocidad` |
| `ControlVelocidad` | `Texto Velocidad` | `Text (TMP)` del Canvas |
| `ControlVelocidad` | `Ruedas` | Las 4 ruedas de la camioneta |
| `ControlVelocidad` | `Movimiento Carretera` | `ControlCarretera` |
| `ControlVelocidad` | `Pivote Aguja` | `Pivote` (RectTransform) |
| `MovimientoCarretera` | `Tramos` | `Tramo1` … `Tramo5` |

## Resultados experimentales

### A) Precisión del servo (3 mediciones por ángulo)

| Ángulo objetivo | Medición 1 | Medición 2 | Medición 3 |
|:-:|:-:|:-:|:-:|
| 130° | 128° | 131° | 129° |
| 140° | 138° | 142° | 139° |
| 120° | 119° | 121° | 122° |
| 100° | 98° | 101° | 99° |
| 90° | 89° | 92° | 91° |

Error máximo: **2°** · Error absoluto medio: **≈ 1,4°** (calculado a partir de las 15 mediciones).

### B) Coherencia y tiempo en cambios de fuente

| Prueba | Transición | Coherencia (±5°) | Tiempo (s) |
|:-:|:-:|:-:|:-:|
| 1 | POT → DIG | Sí | 0,42 |
| 2 | DIG → POT | Sí | 0,48 |
| 3 | POT → DIG | Sí | 0,51 |

Las tres transiciones se mantuvieron dentro de ±5° y con un tiempo medio de **≈ 0,47 s**.

## Solución de problemas

| Síntoma | Causa probable | Solución |
|---|---|---|
| `Error conectando Arduino: …` | Puerto incorrecto u ocupado | Verifica el COM en el Administrador de dispositivos; cierra el Monitor Serie |
| Error `System.IO.Ports` no existe | *Api Compatibility Level* incorrecto | Cámbialo a `.NET Framework` (paso 3) |
| El slider se mueve pero el servo no | Cable de señal / pin / alimentación | Revisa el cableado del servo y la alimentación (5 V y GND comunes) |
| El potenciómetro no actualiza Unity | El sketch no envía `S:<ángulo>,POT` | Comprueba en la consola de Unity los mensajes `Arduino: …` |
| Valores erráticos en el potenciómetro | Ruido del ADC | Promedia varias lecturas o filtra en el sketch |
| La carretera no se mueve | `Tramos` vacío o sin asignar | Asigna los 5 tramos en `MovimientoCarretera` |

## Créditos y licencias

Código propio: `ArduinoSerialController.cs`, `ControlVelocidad.cs` y `MovimientoCarretera.cs`.

Assets de terceros incluidos en el paquete:

| Recurso | Uso | Licencia |
|---|---|---|
| Designersoup — *Low Poly Car Pack Vol. 1* | Vehículos | Según la tienda de origen |
| KayKit — *Forest Nature Pack 1.0 FREE* | Árboles, rocas, arbustos | Ver `License.txt` incluido |
| Avramania — *Props, Tools & Fencing (Free)* | Cercas y props | CC0 1.0 |
| *In Game UI Assets* | Iconos y prefabs de interfaz | Según la tienda de origen |
| *rust-wheel*, *pack_cartoon_cars* | Ruedas y vehículos | Según la fuente |
| *Grass001*, *asphalt_pit_lane*, *kloofendal_48d* (HDRI) | Texturas y cielo | Según la fuente |
| TextMesh Pro | Texto de la interfaz | Unity |

---

## Autores

Autores: Ana Sofia Zuquila Cortes, Nayerlis Rosa Miranda Vega,Nicolas García Soto
Institucion: Universidad de Sanbuenaventura
