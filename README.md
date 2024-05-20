Arttu Yli-Kujala, Aaro Esko and Eeva-kaisa Kohonen
# ( ͡° ͜ʖ ͡°) tech. Smart Mirror

## Preview

![GIF demo](contributions.png)


## 3D modelling
![GIF demo](3Dmodel/videos/1.gif)

## Schematics

![GIF demo](Schematics_and_PCB/Shematics/Schematics.png)

## PCB

![GIF demo](Schematics_and_PCB/PCB_layout/Topside.png)

## Server

```stl
solid cube_corner
  facet normal 0.0 -1.0 0.0
    outer loop
      vertex 0.0 0.0 0.0
      vertex 1.0 0.0 0.0
      vertex 0.0 0.0 1.0
    endloop
  endfacet
  facet normal 0.0 0.0 -1.0
    outer loop
      vertex 0.0 0.0 0.0
      vertex 0.0 1.0 0.0
      vertex 1.0 0.0 0.0
    endloop
  endfacet
  facet normal -1.0 0.0 0.0
    outer loop
      vertex 0.0 0.0 0.0
      vertex 0.0 0.0 1.0
      vertex 0.0 1.0 0.0
    endloop
  endfacet
  facet normal 0.577 0.577 0.577
    outer loop
      vertex 1.0 0.0 0.0
      vertex 0.0 1.0 0.0
      vertex 0.0 0.0 1.0
    endloop
  endfacet
endsolid
```
