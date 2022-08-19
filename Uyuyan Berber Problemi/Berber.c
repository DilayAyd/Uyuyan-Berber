#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define CLIENT_BOUND 20
#define TRIMMING_TIME 1

// Global değişkenler
int musteriCount = 0;
int musteriSeatCount = 0;
int bosMusteriSeatCount = 0;
int berberSeatCount = 0;
int* berberSeats;
int servedMusteri = 0;
int servedMusteriSeat =0;
int totalServedMusteri = 0;

sem_t berber;   //Berber semaforu
sem_t musteri;  //Mşteri semaforu
sem_t mutex;    //Critical section semaforu

//Fonksiyonlar
void AktifBerber(void*);
void AktifMusteri(void*);
void MusteriWaiting();

int main(int argc, char** args)
{
    // Dışarıdan 3 adet veri alınması gerekiyor. Müşteri sayısı, bekleme koltuğu
    //ve berber koltuğu şeklinde. Bu veri sayısının kontrolü sağlanıyor.
	if(argc != 4)
	{
		printf("\nUsage Error\n");
		exit(1);
	}
	
    //Kullanıcıdan alınan değerler değişkenlere atanıyor.
	musteriCount = atoi(args[1]);
	musteriSeatCount = atoi(args[2]);
	berberSeatCount = atoi(args[3]);
	bosMusteriSeatCount = musteriSeatCount;
	berberSeats = (int*) calloc(musteriSeatCount, sizeof(int));
	
    //Tanımlanan müşteri sayısından daha fazla müşteri sayısı girilirse
	if(musteriCount > CLIENT_BOUND)
	{
		printf("\nMusteri Limiti: %d\n",CLIENT_BOUND);
		exit(1);
	}
    
    //Girilen değerler ekrana yazdırılıyor.
	printf("\nMusteri sayisi: \t %d",musteriCount);
	printf("\nMusteri koltuk sayisi: \t %d",musteriSeatCount);
	printf("\nBerber koltugu sayisi: \t %d",berberSeatCount);
	
    //Thread tanımları
	pthread_t berber[berberSeatCount];//Koltuk sayısı arrayinde berber threadi
	pthread_t musteri[musteriCount]; //Müşteri sayısı arrayinde müşteri threadi
	
    //semaforlara başlangıç değerleri veriliyor.
	sem_init(&berber,0,0); //sayan semafor
	sem_init(&musteri,0,0); //sayan semafor
	sem_init(&mutex,0,1); //ikilik semafor
	
	printf("\nBerber Dukkani Acildi!\n");
	
    //Berber threadi başlatılır ve AktifBerber fonksiyonu çağırılır.
	for(int i = 0; i<berberSeatCount; i++)
	{
		pthread_create(&berber[i], NULL,(void*)AktifBerber,(void*)&i);
		sleep(1);
	}

    //Müşteri threadi başlatılır ve AktifMusteri fonksiyonu çağırılır.
	for(int i = 0; i<musteriCount; i++)
	{
		pthread_create(&musteri[i], NULL,(void*)AktifMusteri,(void*)&i);
		MusteriWaiting();
	}

    //Müşteri threadi bitirilir.
	for(int i = 0; i<musteriCount; i++)
	{
		pthread_join(musteri[i],NULL);
	}

    //Toplam tıraş edilen müşteri sayısı ekrana yazdırılır ve dükkanın
    // kapandığı bilgisi verilir.
	printf("Toplam Musteri Sayisi: %d\n",totalServedMusteri);
	printf("\n\nBerber Dukkani Kapandi!\n");
	
	return 0;
}

void AktifBerber(void* counter)
{
	int t = *(int*)counter + 1;
	int nextMusteri, musteriId;
	
	printf("\nBerber %d dukkana geldi.\n",t);
	
	while(1)
	{
		if(!musteriId) //Müşteri yoksa berber uyumaya gider.
			printf("\nBerber %d uyumuya gitti.\n",t);

		sem_wait(&berber);//Berber koltukları dolu ise müşteriler kuyrukta bekletilir.
		sem_wait(&mutex);//Müşterilerin aynı anda girememeleri için
		
        //Bir sonraki müşteri tıraşı için bilgiler güncellernir.
		servedMusteri = (++servedMusteri) % musteriSeatCount;
		nextMusteri = servedMusteri;
		musteriId = berberSeats[nextMusteri];
		berberSeats[nextMusteri] = pthread_self();
		
		sem_post(&mutex);
		sem_post(&musteri);
		
        //Tıraş bilgileri ekrana yazdırılır.
		printf("Musteri %d Berber %d tarafından tıraş ediliyor.\n",musteriId,t);
		sleep(TRIMMING_TIME);
		printf("Berber %d Musteri %d 'in tıraşını tamamladi.\n",t,musteriId);
	}
}

void MusteriWaiting()//Müşteri bekleme süresinin simülize edilmesi
{
	srand((unsigned int)time(NULL));
	usleep(rand() % (250000 - 50000 + 1) + 50000);
}

void AktifMusteri(void* counter)
{
	int c = *(int*)counter+1;
	int servedSeat, berberId;
	while(1)
	{
		sem_wait(&mutex);//Aynı anda dükkana müşteri girmesini engellemek için.
		printf("Musteri %d Berber Dukkanina geldi\n",c);
		
        //Boş koltuk kontrolü
		if(bosMusteriSeatCount > 0)
		{
			bosMusteriSeatCount--;//Müşteri oturur ve boş koltuk sayısı eksiltilir.
			printf("Musteri %d bekliyor.\n",c);
			
			//Müşteri koltuk değerlerinin ayarlanması
            servedMusteriSeat = (++servedMusteriSeat) % musteriSeatCount;
			servedSeat = servedMusteriSeat;
			berberSeats[servedSeat] = c;
			
			sem_post(&berber);//berber uyandırılır.
			sem_post(&mutex);//Critical section biter.
			
			sem_wait(&musteri);//Müşteri tıraş sırasında bekler
			sem_wait(&mutex);//Anı anda tıraşı önlemek için
            
            //Bekleme koltuğundan tıraş koltuğuna oturulduğu için bekleme koltuğu sayısı güncellenir.
            berberId = berberSeats[servedSeat];
            bosMusteriSeatCount++;
            
            //Toplam tıraş edilen müşteri sayısı güncellenir.
            totalServedMusteri++;
            sem_post(&mutex);//Critical section biter.
		}

		else//Boş koltuk yoksa gelen müşteri dükkandan ayrılır.
		{   
			sem_post(&mutex);
			printf("Berber dukkani dolu. Musteri %d dukkandan ayrildi.\n",c);
		}
		pthread_exit(0);
	}
}

