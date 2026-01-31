# Contributing Guidelines

## How to Compile

### Step 1: Install Dependencies
Run the following commands to install the required dependencies and set up the environment:
```bash
sudo apt install libopencv-dev libpam0g-dev libcli11-dev
cd auth
mkdir -p external && cd external
wget -O libtorch.zip https://download.pytorch.org/libtorch/nightly/cpu/libtorch-cxx11-abi-shared-without-deps-2.2.0.dev20231031%2Bcpu.zip
unzip libtorch.zip
rm libtorch.zip
cd ../..
```

### Step 2: Compile the Project
Execute the following commands to compile the project:
```bash
cd auth
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

## Development Guidelines

### Important Warnings
> [!WARNING]
> Editing the `/etc/pam.d/common-auth` file incorrectly may lock you out of your system. 

Before proceeding, ensure the following:
- **Use a Virtual Machine (Recommended)**: For developing and testing PAM modules, it is **strongly recommended** to use a Linux Virtual Machine (e.g., VirtualBox, VMware, or KVM). This allows you to take **snapshots** of your system before making changes, providing an instant recovery path if you get locked out.
- **System Recovery Preparation (Bare OS)**: If you must test on a bare operating system, prepare a bootable USB drive with a Linux distribution. Use the USB drive to boot into a live environment, mount your hard drive, and fix the `common-auth` file if needed.
- **File Permissions**: Grant yourself edit permissions for the `/etc/pam.d/common-auth` file:
   ```bash
   sudo chown -R $(whoami) /etc/pam.d/common-auth
   sudo chmod +w /etc/pam.d/common-auth
   ```

### Post-Compilation Steps
After successfully compiling the project, follow these steps:

1. **Add Your Face**:
   Run the following command to register your face (ensure you are in the project root):
   ```bash
   ./auth/build/cli/facepass register
   ```

2. **Copy the PAM Module**:
   Copy the `libfacepass_pam.so` file to the `/lib/security/` directory:
   ```bash
   sudo cp auth/build/pam/libfacepass_pam.so /lib/security/
   ```

3. **Modify PAM Configuration**:
   Add the following line to the top of the `/etc/pam.d/common-auth` file:
   ```
   auth sufficient libfacepass_pam.so
   ```

4. **Test the PAM Module**:
   Test if the Facepass PAM module works by typing:
   ```bash
   sudo -i
   ```

   If successful, you should be able to log in using facial recognition.
