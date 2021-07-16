define i32 @main() {
entry:
  %0 = alloca i32           ; initialize return_value
  store i32 0, i32* %0
  %1 = alloca [10 x i32]    ; initialize array a[10]
  ; %2 = &a[0]
  %2 = getelementptr [10 x i32], [10 x i32]* %1, i32 0, i32 0
  ; %3 = &a[1]
  %3 = getelementptr [10 x i32], [10 x i32]* %1, i32 0, i32 1
  store i32 10, i32* %2     ; a[0] = 10
  %4 = load i32, i32* %2
  %5 = mul i32 %4, 2        ; a[1] = a[0] * 2
  store i32 %5, i32* %3
  %6 = load i32, i32* %3
  store i32 %6, i32* %0     ; return_value = a[1]
  %7 = load i32, i32* %0
  ret i32 %7
}