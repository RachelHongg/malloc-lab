# 5주차. malloc 구현

## 5.1 과제 목표
'동적 메모리 할당' 방법을 직접 개발하면서 메모리, 포인터 개념에 익숙해지기.

랩 코드를 직접 수정하고, 채점 프로그램을 실행하면서 '내 점수'를 알 수 있습니다.
→ 즉, 나만의 malloc, realloc, free 함수를 구현하는 것!

## 5.2 구현 방법
implicit, explicit, seglist 등 여러 방법이 있지만, 일단 implicit 방법부터 구현해 보겠습니다.
여유가 되면 explicit, seglist, buddy system 까지도 도전해보세요.

## 5.3 구현할 위치
mm.c를 구현하고 mdriver로 채점(테스트) 합니다.
mm_init
mm_malloc
mm_free
mm_realloc

## 5.4 테스트 방법
make 후 ./mdriver 를 실행하면 out of memory 에러 발생
책에 있는 implicit list 방식대로 malloc을 구현해서 해당 에러를 없애기
이후 (시간이 된다면) explicit list와 seglist 등을 활용해 점수를 높이기

Tip: ./mdriver -f traces/binary2-bal.rep 와 같이 특정 세트로만 채점 할 수 있다.

## 5.5 구현에 도움이 되는 자료
컴퓨터 시스템 교재의 9.9장을 찬찬히 읽어가며 진행하자.
교재의 코드를 이해하고 옮겨써서 잘 동작하도록 실행해보는 것이 시작입니다!

과제의 추가적인 사항들은 다음 내용을 참고합니다.
http://csapp.cs.cmu.edu/3e/malloclab.pdf (출처: CMU 카네기멜론대학)
