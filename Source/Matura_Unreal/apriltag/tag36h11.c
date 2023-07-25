/* Copyright (C) 2013-2016, The Regents of The University of Michigan.
All rights reserved.
This software was developed in the APRIL Robotics Lab under the
direction of Edwin Olson, ebolson@umich.edu. This software may be
available under alternative licensing terms; contact the address above.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the Regents of The University of Michigan.
*/

#include <stdlib.h>
#include "tag36h11.h"

static uint64_t codedata[587] = {
	0x0000000d7e00984bUL,
	0x0000000dda664ca7UL,
	0x0000000dc4a1c821UL,
	0x0000000e17b470e9UL,
	0x0000000ef91d01b1UL,
	0x0000000f429cdd73UL,
	0x000000005da29225UL,
	0x00000001106cba43UL,
	0x0000000223bed79dUL,
	0x000000021f51213cUL,
	0x000000033eb19ca6UL,
	0x00000003f76eb0f8UL,
	0x0000000469a97414UL,
	0x000000045dcfe0b0UL,
	0x00000004a6465f72UL,
	0x000000051801db96UL,
	0x00000005eb946b4eUL,
	0x000000068a7cc2ecUL,
	0x00000006f0ba2652UL,
	0x000000078765559dUL,
	0x000000087b83d129UL,
	0x000000086cc4a5c5UL,
	0x00000008b64df90fUL,
	0x00000009c577b611UL,
	0x0000000a3810f2f5UL,
	0x0000000af4d75b83UL,
	0x0000000b59a03fefUL,
	0x0000000bb1096f85UL,
	0x0000000d1b92fc76UL,
	0x0000000d0dd509d2UL,
	0x0000000e2cfda160UL,
	0x00000002ff497c63UL,
	0x000000047240671bUL,
	0x00000005047a2e55UL,
	0x0000000635ca87c7UL,
	0x0000000691254166UL,
	0x000000068f43d94aUL,
	0x00000006ef24bdb6UL,
	0x00000008cdd8f886UL,
	0x00000009de96b718UL,
	0x0000000aff6e5a8aUL,
	0x0000000bae46f029UL,
	0x0000000d225b6d59UL,
	0x0000000df8ba8c01UL,
	0x0000000e3744a22fUL,
	0x0000000fbb59375dUL,
	0x000000018a916828UL,
	0x000000022f29c1baUL,
	0x0000000286887d58UL,
	0x000000041392322eUL,
	0x000000075d18ecd1UL,
	0x000000087c302743UL,
	0x00000008c6317ba9UL,
	0x00000009e40f36d7UL,
	0x0000000c0e5a806aUL,
	0x0000000cc78cb87cUL,
	0x000000012d2f2d01UL,
	0x0000000379f36a21UL,
	0x00000006973f59acUL,
	0x00000007789ea9f4UL,
	0x00000008f1c73e84UL,
	0x00000008dd287a20UL,
	0x000000094a4eee4cUL,
	0x0000000a455379b5UL,
	0x0000000a9e92987dUL,
	0x0000000bd25cb40bUL,
	0x0000000be98d3582UL,
	0x0000000d3d5972b2UL,
	0x000000014c53d7c7UL,
	0x00000004f1796936UL,
	0x00000004e71fed1aUL,
	0x000000066d46fae0UL,
	0x0000000a55abb933UL,
	0x0000000ebee1accaUL,
	0x00000001ad4ba6a4UL,
	0x0000000305b17571UL,
	0x0000000553611351UL,
	0x000000059ca62775UL,
	0x00000007819cb6a1UL,
	0x0000000edb7bc9ebUL,
	0x00000005b2694212UL,
	0x000000072e12d185UL,
	0x0000000ed6152e2cUL,
	0x00000005bcdadbf3UL,
	0x000000078e0aa0c6UL,
	0x0000000c60a0b909UL,
	0x0000000ef9a34b0dUL,
	0x0000000398a6621aUL,
	0x0000000a8a27c944UL,
	0x00000004b564304eUL,
	0x000000052902b4e2UL,
	0x0000000857280b56UL,
	0x0000000a91b2c84bUL,
	0x0000000e91df939bUL,
	0x00000001fa405f28UL,
	0x000000023793ab86UL,
	0x000000068c17729fUL,
	0x00000009fbf3b840UL,
	0x000000036922413cUL,
	0x00000004eb5f946eUL,
	0x0000000533fe2404UL,
	0x000000063de7d35eUL,
	0x0000000925eddc72UL,
	0x000000099b8b3896UL,
	0x0000000aace4c708UL,
	0x0000000c22994af0UL,
	0x00000008f1eae41bUL,
	0x0000000d95fb486cUL,
	0x000000013fb77857UL,
	0x00000004fe0983a3UL,
	0x0000000d559bf8a9UL,
	0x0000000e1855d78dUL,
	0x0000000fec8daaadUL,
	0x000000071ecb6d95UL,
	0x0000000dc9e50e4cUL,
	0x0000000ca3a4c259UL,
	0x0000000740d12bbfUL,
	0x0000000aeedd18e0UL,
	0x0000000b509b9c8eUL,
	0x00000005232fea1cUL,
	0x000000019282d18bUL,
	0x000000076c22d67bUL,
	0x0000000936beb34bUL,
	0x000000008a5ea8ddUL,
	0x0000000679eadc28UL,
	0x0000000a08e119c5UL,
	0x000000020a6e3e24UL,
	0x00000007eab9c239UL,
	0x000000096632c32eUL,
	0x0000000470d06e44UL,
	0x00000008a70212fbUL,
	0x00000000a7e4251bUL,
	0x00000009ec762cc0UL,
	0x0000000d8a3a1f48UL,
	0x0000000db680f346UL,
	0x00000004a1e93a9dUL,
	0x0000000638ddc04fUL,
	0x00000004c2fcc993UL,
	0x000000001ef28c95UL,
	0x0000000bf0d9792dUL,
	0x00000006d27557c3UL,
	0x0000000623f977f4UL,
	0x000000035b43be57UL,
	0x0000000bb0c428d5UL,
	0x0000000a6f01474dUL,
	0x00000005a70c9749UL,
	0x000000020ddabc3bUL,
	0x00000002eabd78cfUL,
	0x000000090aa18f88UL,
	0x0000000a9ea89350UL,
	0x00000003cdb39b22UL,
	0x0000000839a08f34UL,
	0x0000000169bb814eUL,
	0x00000001a575ab08UL,
	0x0000000a04d3d5a2UL,
	0x0000000bf7902f2bUL,
	0x0000000095a5e65cUL,
	0x000000092e8fce94UL,
	0x000000067ef48d12UL,
	0x00000006400dbcacUL,
	0x0000000b12d8fb9fUL,
	0x00000000347f45d3UL,
	0x0000000b35826f56UL,
	0x0000000c546ac6e4UL,
	0x000000081cc35b66UL,
	0x000000041d14bd57UL,
	0x00000000c052b168UL,
	0x00000007d6ce5018UL,
	0x0000000ab4ed5edeUL,
	0x00000005af817119UL,
	0x0000000d1454b182UL,
	0x00000002badb090bUL,
	0x000000003fcb4c0cUL,
	0x00000002f1c28fd8UL,
	0x000000093608c6f7UL,
	0x00000004c93ba2b5UL,
	0x000000007d950a5dUL,
	0x0000000e54b3d3fcUL,
	0x000000015560cf9dUL,
	0x0000000189e4958aUL,
	0x000000062140e9d2UL,
	0x0000000723bc1cdbUL,
	0x00000002063f26faUL,
	0x0000000fa08ab19fUL,
	0x00000007955641dbUL,
	0x0000000646b01daaUL,
	0x000000071cd427ccUL,
	0x000000009a42f7d4UL,
	0x0000000717edc643UL,
	0x000000015eb94367UL,
	0x00000008392e6bb2UL,
	0x0000000832408542UL,
	0x00000002b9b874beUL,
	0x0000000b21f4730dUL,
	0x0000000b5d8f24c9UL,
	0x00000007dbaf6931UL,
	0x00000001b4e33629UL,
	0x000000013452e710UL,
	0x0000000e974af612UL,
	0x00000001df61d29aUL,
	0x000000099f2532adUL,
	0x0000000e50ec71b4UL,
	0x00000005df0a36e8UL,
	0x00000004934e4ceaUL,
	0x0000000e34a0b4bdUL,
	0x0000000b7b26b588UL,
	0x00000000f255118dUL,
	0x0000000d0c8fa31eUL,
	0x000000006a50c94fUL,
	0x0000000f28aa9f06UL,
	0x0000000131d194d8UL,
	0x0000000622e3da79UL,
	0x0000000ac7478303UL,
	0x0000000c8f2521d7UL,
	0x00000006c9c881f5UL,
	0x000000049e38b60aUL,
	0x0000000513d8df65UL,
	0x0000000d7c2b0785UL,
	0x00000009f6f9d75aUL,
	0x00000009f6966020UL,
	0x00000001e1a54e33UL,
	0x0000000c04d63419UL,
	0x0000000946e04cd7UL,
	0x00000001bdac5902UL,
	0x000000056469b830UL,
	0x0000000ffad59569UL,
	0x000000086970e7d8UL,
	0x00000008a4b41e12UL,
	0x0000000ad4688e3bUL,
	0x000000085f8f5df4UL,
	0x0000000d833a0893UL,
	0x00000002a36fdd7cUL,
	0x0000000d6a857cf2UL,
	0x00000008829bc35cUL,
	0x00000005e50d79bcUL,
	0x0000000fbb8035e4UL,
	0x0000000c1a95bebfUL,
	0x0000000036b0baf8UL,
	0x0000000e0da964eaUL,
	0x0000000b6483689bUL,
	0x00000007c8e2f4c1UL,
	0x00000005b856a23bUL,
	0x00000002fc183995UL,
	0x0000000e914b6d70UL,
	0x0000000b31041969UL,
	0x00000001bb478493UL,
	0x0000000063e2b456UL,
	0x0000000f2a082b9cUL,
	0x00000008e5e646eaUL,
	0x000000008172f8f6UL,
	0x00000000dacd923eUL,
	0x0000000e5dcf0e2eUL,
	0x0000000bf9446baeUL,
	0x00000004822d50d1UL,
	0x000000026e710bf5UL,
	0x0000000b90ba2a24UL,
	0x0000000f3b25aa73UL,
	0x0000000809ad589bUL,
	0x000000094cc1e254UL,
	0x00000005334a3adbUL,
	0x0000000592886b2fUL,
	0x0000000bf64704aaUL,
	0x0000000566dbf24cUL,
	0x000000072203e692UL,
	0x000000064e61e809UL,
	0x0000000d7259aad6UL,
	0x00000007b924aedcUL,
	0x00000002df2184e8UL,
	0x0000000353d1eca7UL,
	0x0000000fce30d7ceUL,
	0x0000000f7b0f436eUL,
	0x000000057e8d8f68UL,
	0x00000008c79e60dbUL,
	0x00000009c8362b2bUL,
	0x000000063a5804f2UL,
	0x00000009298353dcUL,
	0x00000006f98a71c8UL,
	0x0000000a5731f693UL,
	0x000000021ca5c870UL,
	0x00000001c2107fd3UL,
	0x00000006181f6c39UL,
	0x000000019e574304UL,
	0x0000000329937606UL,
	0x0000000043d5c70dUL,
	0x00000009b18ff162UL,
	0x00000008e2ccfebfUL,
	0x000000072b7b9b54UL,
	0x00000009b71f4f3cUL,
	0x0000000935d7393eUL,
	0x000000065938881aUL,
	0x00000006a5bd6f2dUL,
	0x0000000a19783306UL,
	0x0000000e6472f4d7UL,
	0x000000081163df5aUL,
	0x0000000a838e1cbdUL,
	0x0000000982748477UL,
	0x0000000050c54febUL,
	0x00000000d82fbb58UL,
	0x00000002c4c72799UL,
	0x000000097d259ad6UL,
	0x000000022d9a43edUL,
	0x0000000fdb162a9fUL,
	0x00000000cb4a727dUL,
	0x00000004fae2e371UL,
	0x0000000535b5be8bUL,
	0x000000048795908aUL,
	0x0000000ce7c18962UL,
	0x00000004ea154d80UL,
	0x000000050c064889UL,
	0x00000008d97fc75dUL,
	0x0000000c8bd9ec61UL,
	0x000000083ee8e8bbUL,
	0x0000000c8431419aUL,
	0x00000001aa78079dUL,
	0x00000008111aa4a5UL,
	0x0000000dfa3a69feUL,
	0x000000051630d83fUL,
	0x00000002d930fb3fUL,
	0x00000002133116e5UL,
	0x0000000ae5395522UL,
	0x0000000bc07a4e8aUL,
	0x000000057bf08ba0UL,
	0x00000006cb18036aUL,
	0x0000000f0e2e4b75UL,
	0x00000003eb692b6fUL,
	0x0000000d8178a3faUL,
	0x0000000238cce6a6UL,
	0x0000000e97d5cdd7UL,
	0x0000000fe10d8d5eUL,
	0x0000000b39584a1dUL,
	0x0000000ca03536fdUL,
	0x0000000aa61f3998UL,
	0x000000072ff23ec2UL,
	0x000000015aa7d770UL,
	0x000000057a3a1282UL,
	0x0000000d1f3902dcUL,
	0x00000006554c9388UL,
	0x0000000fd01283c7UL,
	0x0000000e8baa42c5UL,
	0x000000072cee6adfUL,
	0x0000000f6614b3faUL,
	0x000000095c3778a2UL,
	0x00000007da4cea7aUL,
	0x0000000d18a5912cUL,
	0x0000000d116426e5UL,
	0x000000027c17bc1cUL,
	0x0000000b95b53bc1UL,
	0x0000000c8f937a05UL,
	0x0000000ed220c9bdUL,
	0x00000000c97d72abUL,
	0x00000008fb1217aeUL,
	0x000000025ca8a5a1UL,
	0x0000000b261b871bUL,
	0x00000001bef0a056UL,
	0x0000000806a51179UL,
	0x0000000eed249145UL,
	0x00000003f82aecebUL,
	0x0000000cc56e9acfUL,
	0x00000002e78d01ebUL,
	0x0000000102cee17fUL,
	0x000000037caad3d5UL,
	0x000000016ac5b1eeUL,
	0x00000002af164eceUL,
	0x0000000d4cd81dc9UL,
	0x000000012263a7e7UL,
	0x000000057ac7d117UL,
	0x00000009391d9740UL,
	0x00000007aedaa77fUL,
	0x00000009675a3c72UL,
	0x0000000277f25191UL,
	0x0000000ebb6e64b9UL,
	0x00000007ad3ef747UL,
	0x000000012759b181UL,
	0x0000000948257d4dUL,
	0x0000000b63a850f6UL,
	0x00000003a52a8f75UL,
	0x00000004a019532cUL,
	0x0000000a021a7529UL,
	0x0000000cc661876dUL,
	0x00000004085afd05UL,
	0x0000000e7048e089UL,
	0x00000003f979cdc6UL,
	0x0000000d9da9071bUL,
	0x0000000ed2fc5b68UL,
	0x000000079d64c3a1UL,
	0x0000000fd44e2361UL,
	0x00000008eea46a74UL,
	0x000000042233b9c2UL,
	0x0000000ae4d1765dUL,
	0x00000007303a094cUL,
	0x00000002d7033abeUL,
	0x00000003dcc2b0b4UL,
	0x00000000f0967d09UL,
	0x000000006f0cd7deUL,
	0x000000009807aca0UL,
	0x00000003a295cad3UL,
	0x00000002b106b202UL,
	0x00000003f38a828eUL,
	0x000000078af46596UL,
	0x0000000bda2dc713UL,
	0x00000009a8c8c9d9UL,
	0x00000006a0f2ddceUL,
	0x0000000a76af6fe2UL,
	0x0000000086f66fa4UL,
	0x0000000d52d63f8dUL,
	0x000000089f7a6e73UL,
	0x0000000cc6b23362UL,
	0x0000000b4ebf3c39UL,
	0x0000000564f300faUL,
	0x0000000e8de3a706UL,
	0x000000079a033b61UL,
	0x0000000765e160c5UL,
	0x0000000a266a4f85UL,
	0x0000000a68c38c24UL,
	0x0000000dca0711fbUL,
	0x000000085fba85baUL,
	0x000000037a207b46UL,
	0x0000000158fcc4d0UL,
	0x00000000569d79b3UL,
	0x00000007b1a25555UL,
	0x0000000a8ae22468UL,
	0x00000007c592bdfdUL,
	0x00000000c59a5f66UL,
	0x0000000b1115daa3UL,
	0x0000000f17c87177UL,
	0x00000006769d766bUL,
	0x00000002b637356dUL,
	0x000000013d8685acUL,
	0x0000000f24cb6ec0UL,
	0x00000000bd0b56d1UL,
	0x000000042ff0e26dUL,
	0x0000000b41609267UL,
	0x000000096f9518afUL,
	0x0000000c56f96636UL,
	0x00000004a8e10349UL,
	0x0000000863512171UL,
	0x0000000ea455d86cUL,
	0x0000000bd0e25279UL,
	0x0000000e65e3f761UL,
	0x000000036c84a922UL,
	0x000000085fd1b38fUL,
	0x0000000657c91539UL,
	0x000000015033fe04UL,
	0x000000009051c921UL,
	0x0000000ab27d80d8UL,
	0x0000000f92f7d0a1UL,
	0x00000008eb6bb737UL,
	0x000000010b5b0f63UL,
	0x00000006c9c7ad63UL,
	0x0000000f66fe70aeUL,
	0x0000000ca579bd92UL,
	0x0000000956198e4dUL,
	0x000000029e4405e5UL,
	0x0000000e44eb885cUL,
	0x000000041612456cUL,
	0x0000000ea45e0abfUL,
	0x0000000d326529bdUL,
	0x00000007b2c33cefUL,
	0x000000080bc9b558UL,
	0x00000007169b9740UL,
	0x0000000c37f99209UL,
	0x000000031ff6dab9UL,
	0x0000000c795190edUL,
	0x0000000a7636e95fUL,
	0x00000009df075841UL,
	0x000000055a083932UL,
	0x0000000a7cbdf630UL,
	0x0000000409ea4ef0UL,
	0x000000092a1991b6UL,
	0x00000004b078dee9UL,
	0x0000000ae18ce9e4UL,
	0x00000005a6e1ef35UL,
	0x00000001a403bd59UL,
	0x000000031ea70a83UL,
	0x00000002bc3c4f3aUL,
	0x00000005c921b3cbUL,
	0x0000000042da05c5UL,
	0x00000001f667d16bUL,
	0x0000000416a368cfUL,
	0x0000000fbc0a7a3bUL,
	0x00000009419f0c7cUL,
	0x000000081be2fa03UL,
	0x000000034e2c172fUL,
	0x000000028648d8aeUL,
	0x0000000c7acbb885UL,
	0x000000045f31eb6aUL,
	0x0000000d1cfc0a7bUL,
	0x000000042c4d260dUL,
	0x0000000cf6584097UL,
	0x000000094b132b14UL,
	0x00000003c5c5df75UL,
	0x00000008ae596fefUL,
	0x0000000aea8054ebUL,
	0x00000000ae9cc573UL,
	0x0000000496fb731bUL,
	0x0000000ebf105662UL,
	0x0000000af9c83a37UL,
	0x0000000c0d64cd6bUL,
	0x00000007b608159aUL,
	0x0000000e74431642UL,
	0x0000000d6fb9d900UL,
	0x0000000291e99de0UL,
	0x000000010500ba9aUL,
	0x00000005cd05d037UL,
	0x0000000a87254fb2UL,
	0x00000009d7824a37UL,
	0x00000008b2c7b47cUL,
	0x000000030c788145UL,
	0x00000002f4e5a8beUL,
	0x0000000badb884daUL,
	0x0000000026e0d5c9UL,
	0x00000006fdbaa32eUL,
	0x000000034758eb31UL,
	0x0000000565cd1b4fUL,
	0x00000002bfd90fb0UL,
	0x0000000093052a6bUL,
	0x0000000d3c13c4b9UL,
	0x00000002daea43bfUL,
	0x0000000a279762bcUL,
	0x0000000f1bd9f22cUL,
	0x00000004b7fec94fUL,
	0x0000000545761d5aUL,
	0x00000007327df411UL,
	0x00000001b52a442eUL,
	0x000000049b0ce108UL,
	0x000000024c764bc8UL,
	0x0000000374563045UL,
	0x0000000a3e8f91c6UL,
	0x00000000e6bd2241UL,
	0x0000000e0e52ee3cUL,
	0x000000007e8e3caaUL,
	0x000000096c2b7372UL,
	0x000000033acbdfdaUL,
	0x0000000b15d91e54UL,
	0x0000000464759ac1UL,
	0x00000006886a1998UL,
	0x000000057f5d3958UL,
	0x00000005a1f5c1f5UL,
	0x00000000b58158adUL,
	0x0000000e712053fbUL,
	0x00000005352ddb25UL,
	0x0000000414b98ea0UL,
	0x000000074f89f546UL,
	0x000000038a56b3c3UL,
	0x000000038db0dc17UL,
	0x0000000aa016a755UL,
	0x0000000dc72366f5UL,
	0x00000000cee93d75UL,
	0x0000000b2fe7a56bUL,
	0x0000000a847ed390UL,
	0x00000008713ef88cUL,
	0x0000000a217cc861UL,
	0x00000008bca25d7bUL,
	0x0000000455526818UL,
	0x0000000ea3a7a180UL,
	0x0000000a9536e5e0UL,
	0x00000009b64a1975UL,
	0x00000005bfc756bcUL,
	0x0000000046aa169bUL,
	0x000000053a17f76fUL,
	0x00000004d6815274UL,
	0x0000000cca9cf3f6UL,
	0x00000004013fcb8bUL,
	0x00000003d26cdfa5UL,
	0x00000005786231f7UL,
	0x00000007d4ab09abUL,
	0x0000000960b5ffbcUL,
	0x00000008914df0d4UL,
	0x00000002fc6f2213UL,
	0x0000000ac235637eUL,
	0x0000000151b28ed3UL,
	0x000000046f79b6dbUL,
	0x00000001382e0c9fUL,
	0x000000053abf983aUL,
	0x0000000383c47adeUL,
	0x00000003fcf88978UL,
	0x0000000eb9079df7UL,
	0x000000009af0714dUL,
	0x0000000da19d1bb7UL,
	0x00000009a02749f8UL,
	0x00000001c62dab9bUL,
	0x00000001a137e44bUL,
	0x00000002867718c7UL,
	0x000000035815525bUL,
	0x00000007cd35c550UL,
	0x00000002164f73a0UL,
	0x0000000e8b772fe0UL,
};
apriltag_family_t *tag36h11_create()
{
	apriltag_family_t *tf = calloc(1, sizeof(apriltag_family_t));
	tf->name = strdup("tag36h11");
	tf->h = 11;
	tf->ncodes = 587;
	tf->codes = codedata;
	tf->nbits = 36;
	tf->bit_x = calloc(36, sizeof(uint32_t));
	tf->bit_y = calloc(36, sizeof(uint32_t));
	tf->bit_x[0] = 1;
	tf->bit_y[0] = 1;
	tf->bit_x[1] = 2;
	tf->bit_y[1] = 1;
	tf->bit_x[2] = 3;
	tf->bit_y[2] = 1;
	tf->bit_x[3] = 4;
	tf->bit_y[3] = 1;
	tf->bit_x[4] = 5;
	tf->bit_y[4] = 1;
	tf->bit_x[5] = 2;
	tf->bit_y[5] = 2;
	tf->bit_x[6] = 3;
	tf->bit_y[6] = 2;
	tf->bit_x[7] = 4;
	tf->bit_y[7] = 2;
	tf->bit_x[8] = 3;
	tf->bit_y[8] = 3;
	tf->bit_x[9] = 6;
	tf->bit_y[9] = 1;
	tf->bit_x[10] = 6;
	tf->bit_y[10] = 2;
	tf->bit_x[11] = 6;
	tf->bit_y[11] = 3;
	tf->bit_x[12] = 6;
	tf->bit_y[12] = 4;
	tf->bit_x[13] = 6;
	tf->bit_y[13] = 5;
	tf->bit_x[14] = 5;
	tf->bit_y[14] = 2;
	tf->bit_x[15] = 5;
	tf->bit_y[15] = 3;
	tf->bit_x[16] = 5;
	tf->bit_y[16] = 4;
	tf->bit_x[17] = 4;
	tf->bit_y[17] = 3;
	tf->bit_x[18] = 6;
	tf->bit_y[18] = 6;
	tf->bit_x[19] = 5;
	tf->bit_y[19] = 6;
	tf->bit_x[20] = 4;
	tf->bit_y[20] = 6;
	tf->bit_x[21] = 3;
	tf->bit_y[21] = 6;
	tf->bit_x[22] = 2;
	tf->bit_y[22] = 6;
	tf->bit_x[23] = 5;
	tf->bit_y[23] = 5;
	tf->bit_x[24] = 4;
	tf->bit_y[24] = 5;
	tf->bit_x[25] = 3;
	tf->bit_y[25] = 5;
	tf->bit_x[26] = 4;
	tf->bit_y[26] = 4;
	tf->bit_x[27] = 1;
	tf->bit_y[27] = 6;
	tf->bit_x[28] = 1;
	tf->bit_y[28] = 5;
	tf->bit_x[29] = 1;
	tf->bit_y[29] = 4;
	tf->bit_x[30] = 1;
	tf->bit_y[30] = 3;
	tf->bit_x[31] = 1;
	tf->bit_y[31] = 2;
	tf->bit_x[32] = 2;
	tf->bit_y[32] = 5;
	tf->bit_x[33] = 2;
	tf->bit_y[33] = 4;
	tf->bit_x[34] = 2;
	tf->bit_y[34] = 3;
	tf->bit_x[35] = 3;
	tf->bit_y[35] = 4;
	tf->width_at_border = 8;
	tf->total_width = 10;
	tf->reversed_border = false;
	return tf;
}

void tag36h11_destroy(apriltag_family_t *tf)
{
	free(tf->bit_x);
	free(tf->bit_y);
	free(tf->name);
	free(tf);
}
