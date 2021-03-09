# Kidito

## Quick Start

```console
$ make
$ ./kidito
```

## [scene.conf](./scene.conf)

| Key           | Description                       |
|---------------|-----------------------------------|
| `frag_shader` | path to the fragment shader       |
| `vert_shader` | path to the vertex shader         |
| `texture`     | path to the image for the texture |

## Controls

| Shortcut                          | Description                                                                          |
|-----------------------------------|--------------------------------------------------------------------------------------|
| <kbd>F5</kbd>                     | Hot-reload [./scene.conf](./scene.conf) and all of the associated with it resources. |
| <kbd>SPACE</kbd>                  | Pause/unpause the time uniform variable in shaders                                   |
| <kbd>←</kbd> / <kbd>→</kbd> | Manually step in time back and forth in the paused mode.                             |

## Objectives

- [x] Generate cube mesh
- [x] Render the cube mesh with perspective projection
- [x] Texture the cube
- [x] Fog
- [x] Simple lighting (the classical one with normals and stuff)
