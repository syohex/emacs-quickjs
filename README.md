# emacs-quickjs

[quickjs](https://bellard.org/quickjs/) binding of Emacs Lisp

## How to Use

```lisp
(quickjs-eval "function fizzbuzz(n) {
    const ret = [];
    for (let i = 1; i <= n; ++i) {
        if (i % 3 == 0 && i % 5 == 0) {
            ret.push('fizzbuzz');
        } else if (i % 5 == 0) {
            ret.push('buzz');
        } else if (i % 3 == 0) {
            ret.push('fizz');
        } else {
            ret.push(i);
        }
    }

    return ret;
}

fizzbuzz(15);
")
;; => [1 2 "fizz" 4 "buzz" "fizz" 7 8 "fizz" "buzz" 11 "fizz" 13 14 "fizzbuzz"]
```

## License

- quickjs: MIT License
- This module: GPLv3
