cd vm
cmd.exe /C make_vm.bat
copy *.exe .. /y
del *.exe
cd ..

cd vm-mini
cmd.exe /C make_vm.bat
copy *.exe .. /y
del *.exe
cd ..

cd vm-m32
cmd.exe /C make_vm.bat
copy *.exe .. /y
del *.exe
cd ..
