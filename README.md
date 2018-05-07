![Slow motion](demo/slow-motion.gif)

"Appear 2" is a KWin effect that animates appearing of windows.
Also, this effect is known as Glide 2 (from Compiz).

## Installation

### Arch Linux

For Arch Linux [kwin-effects-appear2](https://aur.archlinux.org/packages/kwin-effects-appear2/)
is available in the AUR.

### Fedora

```sh
sudo dnf copr enable zzag/kwin-effects
sudo dnf refresh
sudo dnf install kwin-effects-appear2
```

### Ubuntu

```sh
sudo add-apt-repository ppa:vladzzag/kwin-effects
sudo apt install libkwin4-effect-appear2
```

### From source

```sh
git clone https://github.com/zzag/kwin-effects-appear2.git
cd kwin-effects-appear2
mkdir build && cd build
cmake ..
make -jN
sudo make install
```
