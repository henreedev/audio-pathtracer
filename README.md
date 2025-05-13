
# FrequenSee Team
### *Interactive Sound Propagation with Bidirectional Path Tracing*

## Group Members
- **Emre Arslan** – Convolutional reverb, occlusion handling  
- **Arman Mohammadi** – Material modeling, BDPT integration, particle system  
- **Henry Earnest** – Unreal Engine integration, BDPT core, visualization  
- **Is Ali Muhammad** – Paper interpretation and algorithm design

---

## Implemented Paper  
**Title:** *Interactive Sound Propagation with Bidirectional Path Tracing*  
**Authors:** Chunxiao Cao, Zhong Ren, Carl Schissler, Dinesh Manocha, Kun Zhou  
**Institutions:** Zhejiang University, University of North Carolina at Chapel Hill  
[ACM Paper Link](https://dl.acm.org/doi/10.1145/3072959.3073701)

---

##  Overview

**FrequenSee** brings physically accurate, real-time spatial audio simulation to Unreal Engine using **Bidirectional Path Tracing (BDPT)**. Inspired by the academic paper *Interactive Sound Propagation with Bidirectional Path Tracing*, our system models how sound propagates through space by tracing rays from both **sources** and **listeners**, accumulating energy over time to generate **impulse responses**, which are then convolved with audio to produce realistic reverb.

Our implementation works dynamically — reverb updates based on the **geometry**, **materials**, and **positions** of audio sources and listeners in the scene. Rather than using static audio effects, **reverb emerges directly from the simulation**, providing a reactive and immersive acoustic experience.

---

## ⚙️ System Overview

The system is built on top of **Unreal Engine**, using two primary custom components:

### Custom Audio Component
- Runs every frame
- Emits **bidirectional rays** from source and listener
- Computes **energy response** from reflections, accounting for:
  - **Distance attenuation**
  - **Material absorption**
  - **Air absorption**
- Fills a 1-second **energy buffer**, which is transformed into an **impulse response**

### Reverb Plugin (FFT Convolution)
- Takes the impulse response and performs **real-time FFT convolution**
- Applies the reverb to **dry audio input**
- Updates as the environment or listener changes
- Runs in sync with the UE audio thread for interactive feedback

### Offline Python Convolution Script
- Allows processing of **entire audio files** using simulated IRs
- Integrated with the Unreal Engine plugin to import and export IRs and dry or wet signals
- Useful for testing or generating reverberant audio clips outside Unreal

---


##  Demo and Results
### Audio Simulation 
![](audio_sim.gif)
###  BDPT Path Visualization
[▶️ Watch BDPT Path Visualization](https://drive.google.com/file/d/1Qrgv6f0Y09TMZDW3rkjjtH8V7Pt_5BQ-/view?resourcekey)

###  Impulse Response Examples

| Environment        | Original Sound | Convolved Sound |
|--------------------|----------------|------------------|
| Doorway            | https://drive.google.com/file/d/1XOndiOHBVuY9yK_lla46qoZtrXG0aPUq/view?resourcekey              | https://drive.google.com/file/d/1hZD_B0aA5joJuC6S6rYDFYOXpanm1xrb/view?resourcekey&usp=slides_web                |
| Corner             | https://drive.google.com/file/d/1XOndiOHBVuY9yK_lla46qoZtrXG0aPUq/view?resourcekey              | https://drive.google.com/file/d/1T59mQEE5fXFiT9d9D4Wz23W0ZxEFHOwn/view?resourcekey&usp=slides_web                |
| Glass Room (Near)  | https://drive.google.com/file/d/1XOndiOHBVuY9yK_lla46qoZtrXG0aPUq/view?resourcekey              | https://drive.google.com/file/d/15T34nr0x890sp03hEvnSHFjNucYuXdPc/view?resourcekey&usp=slides_web                |
| Open Room          | https://drive.google.com/file/d/1XOndiOHBVuY9yK_lla46qoZtrXG0aPUq/view?resourcekey              | https://drive.google.com/file/d/19z8GR203gMbByVdawnfPsJsGzNlRR4gb/view?resourcekey&usp=slides_web                |
| Small Room         | https://drive.google.com/file/d/1XOndiOHBVuY9yK_lla46qoZtrXG0aPUq/view?resourcekey              | https://drive.google.com/file/d/1-mJ5ThKbTr5upl-ekghMefR-14COYp0c/view?resourcekey                |

---

##  Presentation Slides
 [Google Slides - FrequenSee Presentation](https://docs.google.com/presentation/d/1pvNVEQrmCaf1WTmIF3TLoNcuW1mXzVltQuCydXzlHAo/edit?usp=sharing)  

---

## Contributions Summary

| Member         | Contributions                                  |
|----------------|------------------------------------------------|
| **Henry**      | UE integration, visualizations, BDPT core      |
| **Emre**       | Convolutional reverb, occlusion logic          |
| **Arman**      | Material properties, BDPT support, particles   |
| **Ali**        | Paper interpretation and algorithm design      |

---


## Future Work

- **Bug Fixes and Stability Improvements**  
  - Addressing **energy accumulation inaccuracies**  
  - Fixing **wet signal crackling** in FFT convolution  
  - Resolving mismatches between **exported impulse responses** and real-time results

- **Advanced Spatialization**  
  - Integrating **HRTFs (Head-Related Transfer Functions)** for directional, 3D audio rendering  
  - Supporting **binaural rendering** based on listener orientation and path directionality

- **Expanded Material Modeling**  
  - Support for a **broader range of acoustic materials** with frequency-dependent absorption curves  
  - Tools to define and visualize material properties in the editor

- **Multiple Sound Source Support**  
  - Simultaneous simulation and convolution for **multiple active emitters** in the scene  
  - Managing overlapping impulse responses and reverb tails efficiently

---

## Known Bugs

- **Slight crackling in the wet signal**  
  Occasionally audible in the convolved audio output. This is likely due to minor instability in the real-time FFT convolution or insufficient overlap handling.

- **Energy accumulation inaccuracy**  
  There's a known issue in how total energy/IR is accumulated or normalized over time. We are not able to consistenly produce expected results.

- **Mismatch between simulation and convolved output**  
  When combining our BDPT-generated energy response with the real-time convolution, the result does not always match expectations. For example:
  - The BDPT simulation alone works and **can be visualized** correctly.
  - The **FFT convolution engine** works correctly when tested with a **known impulse response** (e.g., a downloaded ground truth IR).
  - But when using our **exported impulse response** with the offline Python script, the result is **overly reverberant**, and when using our own plugin, it has significant artifacts. 
Collapse
