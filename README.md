Build `bgcut` with:
```sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```
You'll need boost and opencv installed.

Run `bgcut` as:
```
./bgcut --image path-to-image
```

After `bgcut` is up and running select a rectangular area you want to keep as a foreground and press `n`.  
To revert changes press `c`. SHIFT/CTRL-click on area to mark it as a background/foreground.  
Press `n` to apply your changes. Press `s` to save foreground image, result image is `path-to-image.bgcut.png`.
