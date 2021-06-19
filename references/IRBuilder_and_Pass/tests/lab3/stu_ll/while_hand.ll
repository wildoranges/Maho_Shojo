define i32 @main() {
entry:
  %0 = alloca i32           ; initialize return_value
  store i32 0, i32* %0
  %1 = alloca i32           ; %1 = &a
  %2 = alloca i32           ; %2 = &i
  store i32 10, i32* %1     ; a = 10
  store i32 0, i32* %2      ; i = 0
  br label %while_entry
while_entry:
  %3 = load i32, i32* %2    ; (i < 10)?
  %4 = icmp slt i32 %3, 10
  br i1 %4, label %trueBB, label %falseBB
end_entry:
  %5 = load i32, i32* %1    ; load a
  store i32 %5, i32* %0     ; store a to &return_value
  %6 = load i32, i32* %0    ; return
  ret i32 %6
trueBB:
  %7 = add i32 %3, 1        ; i = i + 1
  store i32 %7, i32* %2
  %8 = load i32, i32* %1    ; a = a + i
  %9 = add i32 %8, %7
  store i32 %9, i32* %1
  br label %while_entry     ; loop
falseBB:
  br label %end_entry       ; end
}