# TP Actionneur et Automatique appliquée — ESE 2026-2027

Commande d'une machine à courant continu (MCC) par un hacheur 4 transistors piloté par une **Nucleo-STM32G474RE**.

| | |
|---|---|
| **Binôme** | Talha FAYYAZ — Iprahim TURGUT |
| **Enseignant** | alexis.martin@ensea.fr |
| **Avancement** | §1 → §7.3 |

---

## Sommaire
1. [Organisation du dépôt](#1-organisation-du-dépôt)
2. [Mise en place du projet](#2-mise-en-place-du-projet)
3. [Architecture logicielle](#3-architecture-logicielle)
4. [Shell](#4-shell)
5. [6.1 Génération de 4 PWM](#5-61-génération-de-4-pwm)
6. [6.2 Commande de vitesse](#6-62-commande-de-vitesse)
7. [6.3 Premiers tests moteur](#7-63-premiers-tests-moteur)
8. [7.1 Commande de la vitesse avec rampe](#8-71-commande-de-la-vitesse-avec-rampe)
9. [7.2 Mesure de courant](#9-72-mesure-de-courant)
10. [7.3 Mesure de vitesse](#10-73-mesure-de-vitesse)

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
| `SETCCR XXXX` | Fixe le rapport cyclique immédiatement (voir §6.2) |
| `start` | Démarre les PWM à 50 % (voir §7.1) |
| `stop` | Arrête les PWM (voir §7.1) |
| `speed XXXX` | Atteint le CCR demandé avec une rampe (voir §7.1) |
| `current` | Affiche les courants des bras U et V (voir §7.2) |
| `izero` | Enregistre le zéro des capteurs de courant (voir §7.2) |
| `enc` | Affiche la valeur brute du compteur du codeur (voir §7.3) |
| `rpm` | Affiche la vitesse du moteur en tr/min (voir §7.3) |

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

### Câblage du moteur

![Schéma de câblage du moteur](Images/schema_cablage_moteur.png)

- **Alimentation** : 50 V continu, avec une limitation de courant à 5 A pour protéger le montage pendant les essais.
- **Hacheur** : seuls les bras **U** (Q1, Q2) et **V** (Q3, Q4) sont utilisés, ce qui forme un **pont en H**. Les transistors IRF540N sont commandés par TIM1 (PA8, PB13, PA9, PB14) à travers les drivers isolés HCPL-315J.
- **MCC** (48 V / 12 A nominal) : branchée entre les points milieux **U** et **V**. Elle voit donc la tension (2α − 1)·V_bus.
- **Capteurs de courant** LEM GO 10-SME/SP3 sur chaque sortie de bras : I_U → PA1 et I_V → PB1 (voir §7.2).
- **Sur l'arbre** : le codeur incrémental (DE9 J1301 → PA6 / PA4, voir §7.3), la génératrice tachymétrique et la machine synchrone qui sert de charge.

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

---

## 8. 7.1 Commande de la vitesse avec rampe

### Objectif
Ne plus appliquer la consigne d'un coup comme avec `SETCCR`, mais **limiter l'accélération** pour éviter l'appel de courant observé en 6.3.

### Commandes ajoutées
| Commande | Rôle |
|---|---|
| `start` | Fixe le CCR à 2125 (α = 50 %, vitesse nulle) puis démarre les 4 PWM (`HAL_TIM_PWM_Start` et `HAL_TIMEx_PWMN_Start`) |
| `stop` | Arrête les 4 PWM (`HAL_TIM_PWM_Stop` et `HAL_TIMEx_PWMN_Stop`) et remet le CCR à 2125 |
| `speed XXXX` | Définit une **cible** de CCR (0 à 4250). Le CCR rejoint cette cible progressivement. |

**Sécurité :**
- Les PWM ne démarrent plus automatiquement au reset : il faut taper `start`.
- `speed` est refusé tant que le moteur n'a pas été démarré.
- Une valeur hors de [0 ; 4250] est refusée, et rien n'est modifié.

### Principe de la rampe
Le programme conserve deux valeurs :
- `ccr_current` : le CCR appliqué actuellement dans TIM1 ;
- `ccr_target` : le CCR demandé par `speed`.

La commande `speed` ne modifie que la **cible**. La fonction `motor_ramp_update()`, appelée en permanence dans `loop()`, rapproche le CCR actuel de la cible de **`RAMP_STEP` = 5 pas toutes les `RAMP_PERIOD_MS` = 10 ms** (temporisation non bloquante avec `HAL_GetTick()`).

| Transition | Écart de CCR | Durée de la rampe |
|---|---|---|
| 50 % → 70 % (2125 → 2975) | 850 | 850 / 5 × 10 ms = **1,7 s** |
| 50 % → 100 % (2125 → 4250) | 2125 | **4,25 s** |

La pente se règle en modifiant uniquement `RAMP_STEP` et `RAMP_PERIOD_MS` dans `motor.h`.

### Code
**`motor.h`**
```c
#define RAMP_STEP           5                   // pas de CCR
#define RAMP_PERIOD_MS      10                  // ms entre deux pas

void motor_ramp_update(void);
int sh_start(h_shell_t * h_shell, int argc, char ** argv);
int sh_stop(h_shell_t * h_shell, int argc, char ** argv);
int sh_speed(h_shell_t * h_shell, int argc, char ** argv);
```

**`motor.c`** : la rampe
```c
static uint32_t ccr_current = PWM_CCR_STOP;              // CCR appliqué actuellement
static volatile uint32_t ccr_target = PWM_CCR_STOP;      // CCR visé (modifié par le shell)
static volatile uint8_t motor_running = 0;               // 1 si les PWM sont actives

void motor_ramp_update(void)
{
	static uint32_t last_tick = 0;
	uint32_t target = ccr_target;

	if (!motor_running || ccr_current == target)
	{
		return;
	}
	if (HAL_GetTick() - last_tick < RAMP_PERIOD_MS)
	{
		return;
	}
	last_tick = HAL_GetTick();

	if (ccr_current < target)
	{
		motor_apply_ccr((target - ccr_current > RAMP_STEP) ? ccr_current + RAMP_STEP : target);
	}
	else
	{
		motor_apply_ccr((ccr_current - target > RAMP_STEP) ? ccr_current - RAMP_STEP : target);
	}
}
```

**`app.c`**
```c
void loop(){
	motor_ramp_update();
}
```

### Tests
| Commande | Réponse du shell | Comportement observé |
|---|---|---|
| *(reset)* | — | aucune PWM |
| `speed 3000` | `Erreur : moteur arrete, taper start` | rien ne change |
| `start` | `PWM demarrees, CCR = 2125 (50 %)` | 4 PWM à 50 %, moteur à l'arrêt |
| `speed 2975` | `Rampe vers CCR = 2975` | le moteur accélère **progressivement** jusqu'à α = 70 % |
| `speed 2125` | `Rampe vers CCR = 2125` | décélération progressive jusqu'à l'arrêt |
| `speed 1275` | `Rampe vers CCR = 1275` | accélération progressive **dans l'autre sens** (α = 30 %) |
| `speed 5000` | `Erreur : speed <0..4250>, rien n'est modifie` | rien ne change |
| `stop` | `PWM arretees` | PWM coupées |

Le moteur démarre et change de sens **sans à-coup** : la rampe supprime le problème d'appel de courant observé en 6.3.

---

## 9. 7.2 Mesure de courant

### Courants à mesurer
Le moteur est branché entre les bras U et V : **le courant du bras U est le courant du moteur**. Le bras V est traversé par le même courant, de signe opposé (I_V = −I_U), ce qui sert de vérification.

Chaque bras possède un capteur de courant à effet Hall **LEM GO 10-SME/SP3** (U204 pour U, U304 pour V sur le schéma KiCad). Le courant de sortie du bras traverse le capteur avant d'aller vers le moteur.

| Capteur | Courant mesuré | Signal | Broche STM32 | Voie ADC |
|---|---|---|---|---|
| U204 | I_U (courant moteur) | `U_Imes` | **PA1** | ADC1_IN2 |
| U304 | I_V = −I_U | `V_Imes` | **PB1** | ADC1_IN12 |

### Fonction de transfert du capteur
Valeurs relevées dans la datasheet GO-SME/SP3 (tableau *Electrical data*, page 4) :

| Paramètre | Valeur |
|---|---|
| Alimentation U_C | 3,3 V |
| Tension de sortie à 0 A (V_ref) | 1,65 V (± 5 mV d'offset) |
| Sensibilité G | **50 mV/A** |
| Plage de mesure | −25 A à +25 A, soit une sortie de 0,40 V à 2,90 V |

**V_out = 1,65 V + 0,050 × I, donc I = (V_out − V₀) / G**

Chaîne de mesure complète, avec un ADC 12 bits et une référence de 3,3 V :

**V = ADC × 3300 / 4096 (mV) et I = (V − V₀) × 1000 / 50 (mA)**

- Toute la plage du capteur (0,40 à 2,90 V) reste dans la plage de l'ADC (0 à 3,3 V).
- Résolution : 3,3 V / 4096 = 0,8 mV, soit environ **16 mA par pas d'ADC**.

### Première mesure en polling
Le module `acquisition/input_analog.c` réalise la mesure à la demande :
1. `input_analog_read_raw()` choisit la voie, lance une conversion (`HAL_ADC_Start`), **attend** la fin (`HAL_ADC_PollForConversion`) puis lit le résultat (`HAL_ADC_GetValue`). Il n'y a rien à modifier dans CubeMX : la voie est choisie par le code avant chaque conversion.
2. `input_analog_read_avg()` fait la **moyenne de 64 mesures** (environ 1 ms, soit une vingtaine de périodes de PWM) pour lisser l'ondulation de courant due au découpage.
3. La commande `izero`, lancée **moteur à l'arrêt**, mesure la tension réelle de chaque capteur à 0 A et l'enregistre comme V₀. Elle corrige ainsi l'offset propre à chaque capteur.
4. La commande `current` affiche, pour chaque bras, la valeur ADC brute, la tension en mV et le courant en mA.

```c
/* Valeur ADC -> courant (mA) : I = (V - V0) / G */
int32_t input_analog_raw_to_ma(uint32_t raw, int32_t v0_mv)
{
	return (input_analog_raw_to_mv(raw) - v0_mv) * 1000 / CURRENT_SENSITIVITY_MV_PER_A;
}
```

### Tests

**Première version** (une seule mesure par commande, et `izero` ne calibrait que le capteur U) :

![Première mesure de courant](Images/mesure_courant_premiere_version.png)

| Capteur | ADC | Tension | Courant affiché |
|---|---|---|---|
| U | 2063 | 1662 mV | 180 mA |
| V | 2061 | 1660 mV | 140 mA |

Après `izero` (V₀ = 1653 mV), le courant à vitesse nulle aurait dû être proche de 0. Deux défauts expliquent l'écart :
- **Une seule mesure, prise à un instant quelconque** : même à α = 50 %, le courant ondule à chaque commutation autour d'une moyenne nulle. Une mesure isolée peut tomber sur une crête de cette ondulation (9 mV d'écart, soit seulement 11 pas d'ADC).
- **Un seul V₀ pour les deux capteurs** : I_V était calculé avec le zéro du capteur U, alors que chaque capteur a son propre offset.

Le code a donc été amélioré : **moyenne sur 64 mesures** et **un V₀ par capteur**.

**Version améliorée, vitesse nulle** (`start`, α = 50 %), avec la valeur théorique V₀ = 1650 mV :

![Mesure de courant à vitesse nulle](Images/mesure_courant_vitesse_nulle.png)

| Capteur | ADC | Tension | Courant affiché |
|---|---|---|---|
| U | 2057 | 1657 mV | 140 mA |
| V | 2065 à 2068 | 1663 à 1666 mV | 260 à 320 mA |

- Grâce à la moyenne sur 64 mesures, la valeur de I_U est **parfaitement stable** d'une mesure à l'autre.
- À vitesse nulle, le courant moyen doit être nul. Les valeurs affichées correspondent à l'**offset de chaque capteur** : environ 7 mV pour U et 14 mV pour V par rapport aux 1,65 V théoriques. Ces offsets sont supprimés en lançant `izero` moteur arrêté.

**Vitesse non nulle, en polling** (sans `izero`, donc avec V₀ = 1650 mV pour les deux capteurs) :

| α = 68 % (`speed 2900`) | α = 31 % (`speed 1300`) |
|---|---|
| ![Polling CCR 2900](Images/courant_polling_2900.png) | ![Polling CCR 1300](Images/courant_polling_1300.png) |

| Commande | I_U | I_V | Constat |
|---|---|---|---|
| `speed 2900` | +1,5 à +2,3 A | −1,3 à −1,8 A | I_V de signe opposé à I_U |
| `speed 1300` | −0,8 à −1,5 A | +1,5 à +2,1 A | les signes s'inversent avec le sens de rotation |

- Le **sens du courant suit le sens de rotation** : la mesure est validée.
- Deux défauts restent visibles : les offsets des capteurs (I_U + I_V ≠ 0, entre +180 et +560 mA) et une dispersion d'environ ±0,4 A d'une mesure à l'autre. Ces mesures, non synchronisées avec la PWM, tombent à des instants quelconques de l'ondulation de courant. C'est ce que corrige la chaîne d'acquisition suivante.

### Chaîne d'acquisition TIM1 → ADC → DMA

```
TIM1 (PWM 20 kHz) ──TRGO──► ADC1 : PA1 (I_U) puis PB1 (I_V) ──► DMA (circulaire) ──► tableau en mémoire
                                                                  └──► interruption : impulsion sur PC7 + moyenne
```

- **TIM1**, qui génère les PWM, déclenche l'ADC **une fois par période** de PWM (événement Update, Repetition Counter = 1), toujours au même instant : au sommet ou au creux du compteur, loin des commutations. Les mesures sont donc **synchrones avec la PWM**.
- L'ADC convertit 2 voies à la suite (scan) et le **DMA** les recopie en mémoire **sans intervention du CPU**.
- À la fin de chaque séquence, l'interruption `HAL_ADC_ConvCpltCallback` génère une **impulsion sur PC7** (`ADC_SYNC`, broche libre sur la carte du hacheur) et calcule une moyenne sur 64 périodes (3,2 ms).

**Configuration CubeMX**

| Périphérique | Paramètre | Valeur |
|---|---|---|
| TIM1 | Repetition Counter | 1 |
| TIM1 | Trigger Event Selection (TRGO) | Update Event |
| ADC1 | Number Of Conversion | 2 (Rank 1 = IN2/PA1, Rank 2 = IN12/PB1, 47,5 cycles) |
| ADC1 | External Trigger Conversion Source | Timer 1 Trigger Out event (front montant) |
| ADC1 | DMA Continuous Requests | Enabled |
| DMA | ADC1 → DMA1 Channel 1 | Circular, Half Word |
| PC7 | GPIO_Output, label `ADC_SYNC` | Very High speed |

Durée de conversion : 2 × (47,5 + 12,5) cycles à 42,5 MHz ≈ 2,8 µs, très inférieure à la période PWM de 50 µs.

> Problème rencontré : l'ADC était d'abord déclenché par `ADC_EXTERNALTRIG_T3_TRGO` (Timer 3, réservé au codeur et jamais démarré). Aucune conversion n'avait lieu. Corrigé en `ADC_EXTERNALTRIG_T1_TRGO`.

**Code (`acquisition/input_analog.c`)**
```c
static uint16_t adc_dma_buffer[ADC_NB_CHANNELS];    // rempli par le DMA à chaque déclenchement TIM1

void input_analog_init(void)
{
	if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK) Error_Handler();
	if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma_buffer, ADC_NB_CHANNELS) != HAL_OK) Error_Handler();
	...
}

/* Appelée par le DMA quand les 2 conversions sont terminées (1 fois par période PWM) */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef * hadc)
{
	if (hadc->Instance != ADC1) return;
	HAL_GPIO_WritePin(ADC_SYNC_GPIO_Port, ADC_SYNC_Pin, GPIO_PIN_SET);     // impulsion de vérification

	sum_u += adc_dma_buffer[ADC_RANK_I_U];
	sum_v += adc_dma_buffer[ADC_RANK_I_V];
	if (++nb_samples >= ADC_NB_SAMPLES)
	{
		avg_u_raw = sum_u / ADC_NB_SAMPLES;
		avg_v_raw = sum_v / ADC_NB_SAMPLES;
		nb_averages++;
		sum_u = sum_v = nb_samples = 0;
	}

	HAL_GPIO_WritePin(ADC_SYNC_GPIO_Port, ADC_SYNC_Pin, GPIO_PIN_RESET);
}
```
Les commandes `current` et `izero` lisent simplement la dernière moyenne : il n'y a plus d'attente de l'ADC.

**Tests avec la chaîne DMA** (après `izero`) :

![Mesure de courant avec DMA](Images/courant_dma.png)

| Test | I_U | I_V | Constat |
|---|---|---|---|
| Vitesse nulle (`start`, `izero`) | −20 puis 0 mA | −60 puis 0 mA | zéro correct |
| α = 68 % (`speed 2900`) | +2080 → +1560 mA | −2040 → −1520 mA | I_V ≈ −I_U, à 40-80 mA près |
| α = 31 % (`speed 1300`) | −1740 → −1320 mA | +1680 → +1240 mA | les signes s'inversent |

**Comparaison avec le polling :**

| | Polling | TIM1 → ADC → DMA |
|---|---|---|
| Écart I_U + I_V (devrait être 0) | 180 à 560 mA | **40 à 80 mA** |
| Écart entre 2 mesures successives | ± 0,4 A | **≈ 40 mA** (2080 puis 2040 mA) |
| Occupation du CPU | attente de chaque conversion | aucune (DMA) |

La baisse du courant d'une mesure à l'autre (2080 → 1560 mA) n'est pas du bruit : juste après la rampe, le moteur accélère encore et fournit un couple d'inertie. Le courant diminue jusqu'à la valeur de régime permanent (environ 1,5 A à vide), où il ne compense plus que les frottements.

**Validation à l'oscilloscope** (sonde de courant, voie 2 à 500 mA/div, α ≈ 68 %) :

![Courant moteur à l'oscilloscope](Images/courant_moteur_oscillo.jpeg)

- Le courant **moyen** mesuré par l'oscilloscope vaut **1,76 A**, cohérent avec les 1,56 à 2,08 A donnés par la commande `current`.
- On voit nettement l'**ondulation de courant** à chaque commutation (environ 0,6 A entre les curseurs a et b). C'est pour cette raison que la mesure doit être synchronisée sur la PWM et moyennée.

**Vérification de la synchronisation** (`speed 2900`) : D0 = **PC7 (ADC_SYNC)**, D1 à D4 = les 4 PWM, voie 1 = tension du hacheur (20 V/div), voie 2 = courant moteur (1 A/div).

![Synchronisation ADC / PWM](Images/synchro_adc_pc7.jpeg)

- L'impulsion de PC7 (D0) apparaît **une fois par période de PWM**, toujours **au même instant** par rapport aux signaux D1 à D4. Les mesures de courant sont donc bien **synchrones avec la PWM**.
- La mesure tombe loin des fronts de commutation, là où le courant ne présente pas les pics de commutation visibles sur la voie 2.
- L'ondulation de courant sur une période vaut environ **1,26 A crête à crête** (curseurs a et b, Δt = 58,5 µs). C'est pourquoi on moyenne les mesures sur 64 périodes.

> Remarque : la moyenne affichée par l'oscilloscope sur cette capture (−0,35 A) ne correspond pas au courant mesuré par le STM32 au même point de fonctionnement (environ +1,5 A), ni à la capture précédente (1,76 A). La sonde de courant n'était probablement pas remise à zéro, ou était placée dans l'autre sens. Seules l'ondulation et la position de l'impulsion sont exploitées ici.

---

## 10. 7.3 Mesure de vitesse

### Capteur et broches utilisées
Le moteur est équipé d'un **codeur incrémental** à sorties différentielles (A, /A, B, /B, Z, /Z). Il est relié au connecteur **DE9 J1301** de la carte du hacheur, puis passe par le récepteur différentiel **MAX3097** (U1301), alimenté en 3,3 V, qui fournit des signaux logiques au STM32 :

| Signal | Broche STM32 | Fonction |
|---|---|---|
| ENC_A | **PA6** | TIM3_CH1 |
| ENC_B | **PA4** | TIM3_CH2 |
| ENC_Z | PC8 | index (1 impulsion par tour), non utilisé |

> Problème rencontré : sans le câble du codeur branché sur le DE9, le compteur restait à 0 même moteur en marche.

### Fonction de transfert du capteur de vitesse
**TIM3 en mode Encoder TI1 et TI2** compte les fronts montants et descendants des voies A et B (comptage ×4), et compte ou décompte selon le sens de rotation.

- Nombre de pas par tour : N = 4 × PPR = 4 × 1024 = **4096 pas/tour**.
- Vitesse calculée toutes les T = 10 ms :

**N (tr/min) = Δcompteur × 60 000 / (4096 × 10)**, soit une résolution de **1,46 tr/min**.

La différence de deux compteurs 16 bits est convertie en `int16_t`, ce qui gère automatiquement le passage 65535 → 0 et le sens de rotation.

**Génératrice tachymétrique** (mesurée à l'oscilloscope en même temps que `rpm`) :

| Commande | Vitesse codeur (`rpm`) | Tension tachymétrique | Rapport |
|---|---|---|---|
| `speed 2900` | 1149 tr/min | 11,80 V | 10,3 V / 1000 tr/min |
| `speed 1300` | −1224 tr/min | −12,25 V | 10,0 V / 1000 tr/min |

La génératrice tachymétrique donne **K_tach ≈ 10 V / 1000 tr/min** dans les deux sens. Cette valeur standard confirme le réglage **PPR = 1024** du codeur.

### Configuration CubeMX
| Périphérique | Paramètre | Valeur |
|---|---|---|
| TIM3 | Combined Channels | Encoder Mode (TI1 and TI2) |
| TIM3 | Prescaler / Counter Period | 0 / 65535 |
| TIM3 | Input Filter IC1 / IC2 | 5 (filtrage des parasites) |
| TIM7 | Prescaler / Counter Period | 169 / 9999, soit 170 MHz / 170 / 10 000 = **100 Hz (10 ms)** |
| TIM7 | NVIC | TIM7 global interrupt activée |

### Code (`acquisition/input_encoder.c`)
```c
#define ENCODER_PPR             1024
#define ENCODER_COUNTS_PER_REV  (4 * ENCODER_PPR)
#define SPEED_PERIOD_MS         10
#define MS_PER_MIN              60000

/* Appelée toutes les SPEED_PERIOD_MS par l'interruption de TIM7 */
void input_encoder_update(void)
{
	uint16_t count = __HAL_TIM_GET_COUNTER(&htim3);
	int16_t delta = (int16_t)(count - last_count);  // le cast gère le passage 65535 -> 0
	last_count = count;

	speed_rpm = (int32_t)delta * MS_PER_MIN / (ENCODER_COUNTS_PER_REV * SPEED_PERIOD_MS);
}
```
L'appel se fait dans `HAL_TIM_PeriodElapsedCallback` (`main.c`, zone USER CODE), lorsque `htim->Instance == TIM7`.

### Tests

![Mesure de vitesse au codeur](Images/vitesse_codeur.png)

| Commande | α | Vitesse mesurée |
|---|---|---|
| `start` | 50 % | 0 tr/min |
| `speed 2900` | 68,3 % | **1149 tr/min** (stable à ± 1 tr/min) |
| `speed 1300` | 30,6 % | **−1224 tr/min** (sens inverse) |

La vitesse est **proportionnelle à la tension moyenne** appliquée au moteur, (2α − 1)·V_bus :

| | 2α − 1 | Vitesse |
|---|---|---|
| CCR 2900 | 0,365 | 1149 tr/min |
| CCR 1300 | 0,388 | 1224 tr/min |
| **Rapport** | **1,063** | **1,065** |

### Fréquence d'asservissement
La vitesse est calculée toutes les **10 ms (100 Hz)**, par l'interruption de TIM7. C'est aussi la période retenue pour l'asservissement en vitesse.

Ce choix est un compromis :
- une période **plus courte** rendrait l'asservissement plus réactif, mais dégraderait la résolution de la mesure. Le nombre de pas comptés pendant la période diminue : 1,46 tr/min par pas à 10 ms, 14,6 tr/min à 1 ms ;
- une période **plus longue** améliorerait la résolution, mais retarderait la réaction du correcteur.

À 1149 tr/min, le codeur fournit environ 785 pas par période de 10 ms, ce qui donne une mesure précise et stable (± 1 tr/min observé).
