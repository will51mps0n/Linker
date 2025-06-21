local   noop
        add     0       1       2      
        beq     1       2       End
        nor     0       0       0
        lw      0       1       32767
        sw      0       6       Al
        sw      1       2       -32768
        noop
end     halt
Al      .fill   Stack
Jbelly  .fill   Al
Nirbz  .fill    Al
Andy    .fill   Nirbz
End     .fill   10
Gmon    .fill   End