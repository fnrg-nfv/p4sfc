# Functionality test

Simple topology to test the functionality of P4_16.


1. 目前resubmit/recirculate的参数只能是meta/standard_metadata。不能指定只保留哪些参数的值。

2. resubmit走完ingress直接再进parser。recirculate会走完全部过程再进parser，也就是说recirculate能反应对头部的更改而 resubmit不行。

3. 在action中不能使用if语句。（[--Werror=legacy] error: MethodCallStatement: Conditional execution in actions is not supported on this target
）