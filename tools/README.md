# Notice

## Install Media Interface Framework

You might want to install it in default setting,  
and after installation, set all `.exe` files inside `/bin`  
folder as global variables — that is, putting them into the  
Windows System `PATH`.

In this way, you can easily use all useful tools from **MI (Media Interface)**.

---

## MI Usages

Here are some examples you can try from a command line terminal (any location):

### ▶ Open a GUI to view a BMP file

```cmd
mi_viewer pens_rgb.bmp
```

### ▶ Combine MI modules using `mi_pipe2`

#### - Read image

```cmd
mi_pipe2 -i pens_rgb.bmp :: view
```

#### - Crop an image

```cmd
mi_pipe2 -i pens_rgb.bmp -o crop1.bmp -form bmp :: crop_n_shuffle -crop 0.2W 0.2H 0.25W 0.25H
```

#### - View images as repeating video

```cmd
mi_pipe2 :: read_file -f crop1.bmp crop2.bmp crop3.bmp crop4.bmp crop5.bmp :: frame_repeat :: view -play -rate 2
```