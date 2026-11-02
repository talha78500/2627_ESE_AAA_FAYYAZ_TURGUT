# TP Actionneur et Automatique appliquée — ESE 2026-2027

Commande d'une machine à courant continu (MCC) avec un hacheur 4 transistors piloté par une **Nucleo-STM32G474RE**.

| | |
|---|---|
| **Binôme** | Iprahim TURGUT — FAYYAZ |
| **Dépôt** | [IprahimTurgut1/2627_ESE_AAA_TURGUT_FAYYAZ](https://github.com/IprahimTurgut1/2627_ESE_AAA_TURGUT_FAYYAZ), partagé avec alexis.martin@ensea.fr |
| **Avancement** | §1 → §5 ✅ · §6 → §7 à faire |

---

## Sommaire
1. [Organisation du dépôt](#1-organisation-du-dépôt)
2. [Bonnes pratiques appliquées](#2-bonnes-pratiques-appliquées)
3. [Mise en place GitHub / CubeIDE](#3-mise-en-place-github--cubeide)
4. [Architecture logicielle (app.c)](#4-architecture-logicielle-appc)
5. [Shell](#5-shell)

---

## 1. Organisation du dépôt

```
.
├── README.md                ← compte-rendu (complété à chaque séance)
├── docs/                    ← chronogrammes, captures oscilloscope
├── hardware/Inverter_KiCAD  ← schéma KiCad du hacheur (fourni)
└── software/                ← projet STM32CubeIDE (NUCLEO-G474RET6-Inverter_Base)
    └── Core/
        ├── Inc/ Src/
        │   ├── acquisition/     input_analog, input_encoder
        │   ├── motor_control/   motor (PWM / pont), asserv (PID)
        │   └── user_interface/  button, led, shell
        └── app.c                init_device() + loop()
```

## 2. Bonnes pratiques appliquées

- Variables et fonctions en `snake_case`, macros en `MAJUSCULES`.
- **Aucun nombre magique** : toutes les valeurs (fréquence, ARR, temps mort, rapport cyclique de test) sont des `#define` dans `motor.h`, avec leur calcul.
- Les retours des fonctions HAL sont testés (`HAL_TIM_PWM_Start`, `HAL_TIMEx_PWMN_Start`, `HAL_UART_Receive_IT`, `HAL_UART_Transmit`…). En cas d'erreur à l'initialisation, un message est affiché sur le shell.
- **Garde-fou CubeMX** : `motor_init()` relit la configuration réelle de TIM1 (horloge, ARR, mode center-aligned, DTG) et renvoie `HAL_ERROR` si elle ne correspond plus aux `#define`. Si quelqu'un régénère le code avec d'autres valeurs, l'erreur est détectée au démarrage au lieu de produire des PWM fausses.
- Un `_Static_assert` vérifie à la compilation que le temps mort reste dans la zone linéaire de DTG.

## 3. Mise en place GitHub / CubeIDE

1. Base récupérée depuis `https://github.com/DBXYD/2526_MSC_SAC` (dossier `TP/`).
2. Dépôt créé, puis cloné (`git clone <url>`). Les fichiers du prof y ont été copiés et le projet a été ouvert dans CubeIDE.
3. Synchronisation : `git add .` → `git commit -m "..."` → `git push`.
4. `.gitignore` : les dossiers de build (`Debug/`, `Release/`) et les fichiers d'IDE locaux ne sont pas versionnés.

## 4. Architecture logicielle (app.c)

- `main.c` appelle les `MX_xxx_Init()` générés par CubeMX, puis `init_device()`, puis `loop()` en boucle.
- `init_device()` initialise dans l'ordre :
  1. le **shell** (fonctions de transmission/réception UART2, `shell_init`, réception par interruption) ;
  2. la **LED** (commande `led`) ;
  3. le **moteur** (`motor_init()`).
- `HAL_UART_RxCpltCallback` relance la réception d'un caractère puis appelle `shell_run()`. Le shell fonctionne donc en tâche de fond, sans bloquer `loop()`.
- L'UART2 n'apparaît **que** dans `app.c`, pour que les autres modules restent portables.

**Corrections apportées à la base :**

| Fichier | Problème | Correction |
|---|---|---|
| `led.h` / `app.c` | `led_init()` était appelée sans prototype (déclaration implicite) | Prototype ajouté dans `led.h`, `led.h` inclus dans `app.c` |
| `led.c` | `SET`/`RESET` passés à `HAL_GPIO_WritePin` (mauvais type enum) | `GPIO_PIN_SET` / `GPIO_PIN_RESET` |
| `shell.c` | `_` refusé dans les noms de commande : `shell_add("SET_CCR", …)` aurait tronqué une chaîne constante (écriture en flash) | `_` ajouté à `is_character_valid()` (nécessaire pour la §6.2) |

Le projet compile **sans aucun warning** avec `-Wall -Wextra`.

## 5. Shell

Terminal série sur l'USB ST-Link (USART2). Commandes disponibles :

| Commande | Rôle |
|---|---|
| `help` | Liste des commandes |
| `test a b c` | Affiche les arguments reçus (démo `argc`/`argv`) |
| `led on/off/toggle` | Pilote la LED utilisateur (PA5) |

Pour ajouter une commande : écrire une fonction `int ma_cmd(h_shell_t* h_shell, int argc, char** argv)`, puis appeler `shell_add(&hshell1, "nom", ma_cmd, "description")` dans la fonction d'initialisation du module.
