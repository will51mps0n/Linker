        sw     1       2       End //test using local and undef global. 
        sw     0       0       end 
        lw      0       1       Ug1
        lw      0       1       Ug2
        lw      0       1       Ug3
        noop
        halt
end .fill Undef  
Ug1 .fill 5
Ug2 .fill End
Ug3 .fill Undef
