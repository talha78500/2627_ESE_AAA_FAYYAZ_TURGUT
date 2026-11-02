# TP Actionneur et Automatique appliquée — ESE 2026-2027

Commande d'une machine à courant continu (MCC) par un hacheur 4 transistors piloté par une **Nucleo-STM32G474RE**.

| | |
|---|---|
| **Binôme** | Talha FAYYAZ — Iprahim TURGUT |
| **Enseignant** | alexis.martin@ensea.fr |
| **Avancement** | §1 → §6.3 terminés · §7 à faire |

---

## Sommaire
1. [Organisation du dépôt](#1-organisation-du-dépôt)
2. [Mise en place du projet](#2-mise-en-place-du-projet)
3. [Architecture logicielle](#3-architecture-logicielle)
4. [Shell](#4-shell)
5. [6.1 Génération de 4 PWM](#5-61-génération-de-4-pwm)
6. [6.2 Commande de vitesse](#6-62-commande-de-vitesse)
7. [6.3 Premiers tests moteur](#7-63-premiers-tests-moteur)

---

## 1. Organisation du dépôt

```
.
├── README.md                  ← compte-rendu
├── Images/                    ← captures oscilloscope et terminal
├── PDFs/                      ← sujet du TP, datasheet IRF540N
├── TD/                        ← projets des TD
└── TP/
    ├── hardware/Inverter_KiCAD   ← schéma du hacheur
    └── software/Base             ← projet STM32CubeIDE du TP
        └── Core/Src
            ├── acquisition/      input_analog, input_encoder
            ├── motor_control/    motor (PWM), asserv (PID)
            ├── user_interface/   button, led, shell
            └── app.c             init_device() + loop()
```

## 2. Mise en place du projet

- Projet de base récupéré depuis le dépôt de l'enseignant (`DBXYD/2526_MSC_SAC`, dossier `TP/`).
- Projet ouvert dans STM32CubeIDE depuis `TP/software/Base`.
- **Correction de la base** : `led_init()` était appelée dans `app.c` sans prototype, ce qui provoque une erreur *implicit declaration* avec les compilateurs récents. Le prototype a été ajouté dans `led.h` et `led.h` est inclus dans `app.c`.

## 3. Architecture logicielle

- `main.c` initialise les périphériques générés par CubeMX, appelle `init_device()` une fois, puis `loop()` en boucle.
- `init_device()` (dans `app.c`) initialise :
  1. le **shell** sur l'UART2 (réception d'un caractère par interruption) ;
  2. la **LED** (commande `led`) ;
  3. le **moteur** avec `motor_init()`.
- `HAL_UART_RxCpltCallback` relance la réception puis appelle `shell_run()` : le shell tourne en tâche de fond.
- L'UART2 n'est utilisée que dans `app.c`, ce qui rend les autres modules portables.

## 4. Shell

Terminal série via l'USB ST-Link (Tera Term, port COM de la Nucleo).

| Commande | Rôle |
|---|---|
| `help` | Liste des commandes |
| `test a b c` | Affiche les arguments reçus |
| `led on/off/toggle` | Pilote la LED utilisateur |
| `SETCCR XXXX` | Fixe le rapport cyclique (voir §6.2) |

Pour ajouter une commande, on écrit une fonction `int f(h_shell_t*, int argc, char** argv)` puis on l'enregistre avec `shell_add(&hshell1, "nom", f, "description")`.

---

## 5. 6.1 Génération de 4 PWM

### Cahier des charges
| Exigence | Demandé | Obtenu |
|---|---|---|
| Fréquence PWM | 20 kHz | **20,01 kHz** (mesuré) |
| Résolution | ≥ 10 bits | **≈ 12 bits** |
| Temps mort | selon la datasheet des transistors | **118 ns** (DTG = 20), mesuré 122–124 ns |
| Rapport cyclique de test | 60 % | CCR_U = 2550 |

### Broches utilisées (TIM1, déjà attribué)
| Bras | Transistor haut | Transistor bas |
|---|---|---|
| U | TIM1_CH1 → **PA8** | TIM1_CH1N → **PB13** |
| V | TIM1_CH2 → **PA9** | TIM1_CH2N → **PB14** |

### Commande complémentaire décalée
- Bras U : rapport cyclique **α**, bras V : rapport cyclique **1 − α**, soit `CCR_V = PWM_PERIOD − CCR_U`.
- Tension moyenne aux bornes du moteur : **⟨U⟩ = (2α − 1)·V_bus**, d'où :
  - α = 50 % → tension moyenne nulle, moteur à l'arrêt ;
  - α > 50 % → un sens de rotation ;
  - α < 50 % → l'autre sens.
- Le timer est en mode **center-aligned** : les impulsions des bras U et V sont centrées sur le même instant, et le moteur voit une tension découpée à 2 × 20 kHz.

### Fréquence et résolution
- Horloge timer : HSE 24 MHz → PLL → **f_TIM = 170 MHz**, Prescaler = 0.
- En center-aligned, le compteur monte puis redescend (0 → ARR → 0), donc **une période = 2 × ARR ticks**.

  f_PWM = 170 MHz / (2 × ARR), soit ARR ≈ 4250 pour 20 kHz.

- Valeur configurée : **ARR = 4250 − 1 = 4249**, ce qui donne f = 170 MHz / (2 × 4249) = 20,005 kHz. L'oscilloscope mesure **20,01 kHz**.
- Résolution : log₂(4249) ≈ **12 bits** ≥ 10 bits.

### Temps mort
Transistors du hacheur : **IRF540N** (datasheet dans `PDFs/`).

| Paramètre (datasheet) | Valeur |
|---|---|
| t_d(on) — Turn-On Delay Time | 9,5 ns |
| t_d(off) — Turn-Off Delay Time | 40 ns |
| t_f — Fall Time | 55 ns |

Le temps mort doit couvrir le blocage du transistor qui s'éteint, moins le délai d'allumage de l'autre :

**DT_min = t_d(off) + t_f − t_d(on) = 40 + 55 − 9,5 ≈ 86 ns**

Programmation : 1 pas de DTG = 1 / 170 MHz = 5,88 ns.
Valeur choisie : **DTG = 20, soit 20 × 5,88 = 118 ns**, ce qui laisse environ 30 ns de marge.

> Remarque : les grilles sont commandées par un driver opto-isolé HCPL-315J (un par bras, double voie). Sa datasheet donne un écart de propagation entre voies (PDD) de ±350 ns. Si l'enseignant le demande, le temps mort pourra être augmenté jusqu'à environ 500 ns (DTG = 85) pour couvrir cet écart.

### Configuration CubeMX de TIM1
| Paramètre | Valeur |
|---|---|
| Channel 1 / Channel 2 | PWM Generation CH1 CH1N / CH2 CH2N |
| Prescaler | 0 |
| Counter Mode | Center Aligned mode 1 |
| Counter Period | 4250 − 1 |
| Dead Time | 20 |
| Pulse CH1 / CH2 | 2550 / 1700 (60 % / 40 %) |

### Code (`motor_control/motor.c`)
```c
void motor_init(void)
{
	shell_add(&hshell1, "SETCCR", sh_set_ccr, "SETCCR <0..4250> : rapport cyclique");
	motor_set_ccr(PWM_DUTY_TEST);                      // 60 %

	if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK)    Error_Handler();
	if (HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1) != HAL_OK) Error_Handler();
	if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2) != HAL_OK)    Error_Handler();
	if (HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2) != HAL_OK) Error_Handler();
}
```

### Validation à l'oscilloscope

**4 PWM avec la sonde numérique** (D0 = PA8, D1 = PB13, D2 = PA9, D3 = PB14, seuil 1,65 V, 40 µs/div) :

![4 PWM à l'oscilloscope](Images/signaux_4_PWM.jpeg)

- Les signaux haut et bas de chaque bras sont **complémentaires**.
- Les impulsions des bras U et V sont **centrées sur le même instant** : c'est la commande décalée.

**Temps mort, bras U** (PA8 ↓ → PB13 ↑, 200 ns/div) : **Δt = 124 ns**

![Temps mort bras U](Images/deadtime_PA8_PB13_CH1.jpeg)

**Temps mort, bras V** (PA9 ↓ → PB14 ↑, 100 ns/div) : **Δt = 122 ns**

![Temps mort bras V](Images/deadtime_PA9_PB14_CH2.jpeg)

**Deux voies complémentaires** (voies analogiques, CCR = 2000) : **f = 20,01 kHz**

![PWM complémentaires 20 kHz](Images/Valeur%20de%202000%20pour%20crr.jpeg)

| Mesure | Attendu | Mesuré |
|---|---|---|
| Fréquence | 20 kHz (20,005 kHz avec ARR = 4249) | **20,01 kHz** |
| Complémentarité haut / bas | oui | oui |
| Commande décalée (U et V centrés) | oui | oui |
| Temps mort bras U (PA8 → PB13) | 118 ns | **124 ns** |
| Temps mort bras V (PA9 → PB14) | 118 ns | **122 ns** |

L'écart de 4 à 6 ns sur le temps mort vient du temps de montée des sorties GPIO (configurées en vitesse *Low*) et de la résolution de la sonde numérique (2 ns).

---

## 6. 6.2 Commande de vitesse

### Principe
La commande `SETCCR XXXX`, envoyée par l'UART (USB), fixe le rapport cyclique :

1. **Détection de la commande** : le shell compare le premier mot reçu aux commandes enregistrées avec `shell_add`.
2. **Conversion** : `strtol` transforme `XXXX` en entier. La valeur est refusée si elle contient autre chose que des chiffres ou si elle est négative.
3. **Vérification** : la valeur doit être comprise entre **`PWM_CCR_MIN` = 0 et `PWM_CCR_MAX` = 4250 (α = 100 %)**. Si elle est hors plage, un **message d'erreur** est affiché et **rien n'est modifié**.
4. **Application** : `motor_set_ccr()` écrit `CCR_U = ccr` et `CCR_V = PWM_PERIOD − ccr`, ce qui pilote les 4 transistors.

> **Choix de sécurité :** le sujet propose de ramener une valeur trop grande au maximum. Nous avons choisi de **refuser** la valeur à la place : une faute de frappe (par exemple 30000 au lieu de 3000) ne peut ainsi pas envoyer le moteur à pleine vitesse.
>
> Le nom `SETCCR` est utilisé à la place de `SET_CCR`, car le shell de base refuse le caractère `_` (fonction `is_character_valid` dans `shell.c`).

### Code
```c
#define PWM_PERIOD      4250                    // ARR + 1
#define PWM_CCR_MAX     PWM_PERIOD              // alpha = 100 %
#define PWM_CCR_MIN     0                       // alpha = 0 %
#define PWM_CCR_STOP    (PWM_PERIOD / 2)        // alpha = 50 % -> tension moyenne nulle
#define PWM_DUTY_TEST   2550                    // 60 %

HAL_StatusTypeDef motor_set_ccr(uint32_t ccr)
{
	if (ccr > PWM_CCR_MAX) {
		return HAL_ERROR;                       // hors plage : on ne touche à rien
	}
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, PWM_PERIOD - ccr);
	return HAL_OK;
}

int sh_set_ccr(h_shell_t * h_shell, int argc, char ** argv)
{
	int size;

	if (argc != 2) {
		size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "Usage : SETCCR <%d..%d>\r\n", PWM_CCR_MIN, PWM_CCR_MAX);
		h_shell->drv.transmit(h_shell->print_buffer, size);
		return -1;
	}

	char * end;
	long value = strtol(argv[1], &end, 10);     // conversion chaîne -> entier

	if (*end != '\0' || value < PWM_CCR_MIN || value > PWM_CCR_MAX) {
		size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "Erreur : valeur hors plage <%d..%d>, rien n'est modifie\r\n", PWM_CCR_MIN, PWM_CCR_MAX);
		h_shell->drv.transmit(h_shell->print_buffer, size);
		return -1;
	}

	motor_set_ccr((uint32_t)value);
	size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "CCR = %ld\r\n", value);
	h_shell->drv.transmit(h_shell->print_buffer, size);
	return 0;
}
```

### Tests
**Premiers essais** (Tera Term), avec la première version de la commande :

![Test SETCCR dans le terminal](Images/Test%20liasion%20uart%20shell%20crr.png)

**Version finale** (PuTTY), avec confirmation et gestion des erreurs :

![Test SETCCR avec gestion des erreurs](Images/Shell_Testmoteur_6_3.PNG)

| Commande tapée | Réponse du shell | Ce qui est vérifié |
|---|---|---|
| `SETCCR 2125` | `CCR = 2125` | valeur valide appliquée (α = 50 %) |
| `SETCCR 5000` | `Erreur : valeur hors plage <0..4250>, rien n'est modifie` | valeur > max **refusée** |
| `SET` | `SET : no such command` | seule la commande exacte est reconnue |
| `SETCCR2400` | `SETCCR2400 : no such command` | un espace est obligatoire entre la commande et la valeur |
| `SETCCR 2400` | `CCR = 2400` | valeur valide appliquée |

Le rapport cyclique mesuré sur le bras U suit bien la commande :

| Commande | α théorique (CCR / 4249) | α mesuré |
|---|---|---|
| `SETCCR 1000` | 23,5 % | **23,28 %** |
| `SETCCR 2000` | 47,1 % | voies complémentaires à 20,01 kHz (§6.1) |
| `SETCCR 3000` | 70,6 % | **70,35 %** |

L'écart d'environ 0,2 % correspond au temps mort : 118 ns / 50 µs ≈ 0,24 %.

| CCR = 1000 | CCR = 3000 |
|---|---|
| ![CCR 1000](Images/Valeur%20de%201000%20pour%20crr.jpeg) | ![CCR 3000](Images/Valeur%20de%203000%20pour%20crr.jpeg) |

---

## 7. 6.3 Premiers tests moteur

### Protocole
1. Moteur branché en respectant ses données constructeur, signaux vérifiés par l'enseignant.
2. Démarrage à **α = 50 %** (`SETCCR 2125`) : tension moyenne nulle, le moteur reste à l'arrêt.
3. Passage à **α = 70 %** (`SETCCR 2975`).

### Essai à CCR = 2500 (α ≈ 59 %)

![Test moteur à CCR = 2500](Images/test_fonctionnel_6_3_2500CCR.jpeg)

- **D0 à D3** : les 4 PWM restent complémentaires et décalées, moteur branché.
- **Voie 1 (jaune, 20 V/div)** : tension côté puissance pendant la rotation du moteur. On observe des **surtensions et des oscillations à chaque commutation**, dues aux inductances du moteur et du câblage.

### Problème d'un changement brutal de rapport cyclique
Un passage brutal du rapport cyclique (par exemple de 50 % à 70 %) applique d'un coup une forte tension moyenne au moteur, alors qu'il est encore à l'arrêt. Sa force contre-électromotrice est nulle, donc le courant n'est limité que par la résistance de l'induit : c'est un **fort appel de courant**, qui peut déclencher les protections du hacheur et provoquer un à-coup mécanique.

### Solution
Il faut une **montée progressive du rapport cyclique** jusqu'à la valeur demandée, avec un petit incrément du CCR à intervalle de temps régulier. Cette rampe est mise en place avec la commande `speed` au §7.1.
