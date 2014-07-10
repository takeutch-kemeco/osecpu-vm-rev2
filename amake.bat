@rem cc1.exe ÇÕ c:/mingw/libexec/gcc/mingw32/3.4.5/cc1.exe ÇÃÉRÉsÅ[ÇµÇΩÇ‡ÇÃ
@rem c:\mingw\bin\gcc.exe -E -o a_0ask.txt -x c %1.ask
@if not defined OSECPU_EXTOLS set OSECPU_EXTOLS=.\
@%OSECPU_EXTOLS%ext_tols\cc1.exe -quiet -E -o a_0ask.txt %1.ask
@osectols.exe tool:aska in:a_0ask.txt out:a_1oas_%1.txt
@rem c:\mingw\bin\gcc.exe %2 %3 %4 %5 %6 %7 %8 %9 -E -P -o a_2cas.txt -x c a_1oas_%1.txt
@%OSECPU_EXTOLS%ext_tols\cc1.exe -quiet %2 %3 %4 %5 %6 %7 %8 %9 -E -P -o a_2cas.txt a_1oas_%1.txt
@osectols.exe tool:lbstk in:a_2cas.txt out:a_3cas.txt lst:a_3lbl.txt
@osectols.exe tool:db2bin in:a_3cas.txt out:%1.ose
@osectols.exe tool:appack in:%1.ose out:%1_.ose