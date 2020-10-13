;;; test-quickjs.el --- Test for quickjs.el -*- lexical-binding: t -*-

;; Copyright (C) 2020 by Shohei YOSHIDA

;; Author: Shohei YOSHIDA <syohex@gmail.com>

;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.

;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with this program.  If not, see <http://www.gnu.org/licenses/>.

;;; Commentary:

;;; Code:

(require 'ert)
(require 'quickjs)

(ert-deftest eval ()
  "Eval string as JavaScript code"
  (let ((got (quickjs-eval "
function add(a, b) {
    return a + b;
}

add(10, 32);
")))
    (should (= got 42)))

  (let ((got (quickjs-eval "
const arr = [4, 3, 2, 1];
arr.sort((a, b) => a - b);
arr;
")))
    (should (equal got [1 2 3 4]))))

(ert-deftest eval-with-context ()
  "Eval with context"
  (let* ((ctx (quickjs-make-context))
         (_ (quickjs-eval-with-context
             ctx
             "
function concat(a, b) {
  return `${a}${b}`;
}
")))
    (should (string= (quickjs-call ctx 'concat "He" "llo") "Hello"))))

;;; test-quickjs.el ends here
