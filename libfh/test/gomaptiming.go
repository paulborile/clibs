package main

import (
	"fmt"
	"math/rand"
	"time"
)

const (
	NumKeys = 1000000
)

func main() {

	m := make(map[string]int, NumKeys)
	var InsertAvgTime, GetAvgTime, DelAvgTime float64
	var keys []string

	fmt.Printf("Building keys...\n")
	for j := 0; j < NumKeys; j++ {
		keys = append(keys, GenerateRandomStrings(int64(1000+j), 100, 350))
	}

	for i := 0; i < NumKeys; i++ {
		// key := GenerateRandomStrings(int64(1000+i), 100, 350)

		t0 := time.Now()
		m[keys[i]] = 1
		t1 := time.Now()
		dur := int64(t1.Sub(t0))
		InsertAvgTime = computeAverage(InsertAvgTime, i, dur)
	}
	fmt.Printf("%d random keys, size %d-%d, avg insert time %.4f\n", NumKeys, 100, 350, InsertAvgTime/float64(time.Microsecond))

	for i := 0; i < NumKeys; i++ {
		// key := GenerateRandomStrings(int64(1000+i), 100, 350)

		t0 := time.Now()
		if m[keys[i]] != 1 {
			fmt.Printf("something bad happedning\n")
		}
		t1 := time.Now()
		dur := int64(t1.Sub(t0))
		GetAvgTime = computeAverage(GetAvgTime, i, dur)
	}
	fmt.Printf("%d random keys, size %d-%d, avg get time %.4f\n", NumKeys, 100, 350, GetAvgTime/float64(time.Microsecond))

	for i := 0; i < NumKeys; i++ {
		// key := GenerateRandomStrings(int64(1000+i), 100, 350)

		t0 := time.Now()
		delete(m, keys[i])
		t1 := time.Now()
		dur := int64(t1.Sub(t0))
		DelAvgTime = computeAverage(DelAvgTime, i, dur)
	}
	fmt.Printf("%d random keys, size %d-%d, avg del time %.4f\n", NumKeys, 100, 350, DelAvgTime/float64(time.Microsecond))

}

// computeAverage : rolling avg
func computeAverage(currentAvg float64, count int, newValue int64) float64 {

	if count == 0 {
		return float64(newValue)
	}
	return float64(((currentAvg * float64(count)) + float64(newValue)) / (float64(count) + 1))
}

// GenerateRandomStrings
func GenerateRandomStrings(seed int64, minlen, maxlen int) string {
	source := rand.NewSource(seed)
	rng := rand.New(source)

	return RandomString(rng, minlen, maxlen)
}

// RandomString
func RandomString(rng *rand.Rand, minlen, maxlen int) string {
	chars := make([]rune, rng.Intn(maxlen-minlen)+minlen)
	for i := range chars {
		chars[i] = rune(rng.Intn('}'-'0') + '0')
	}
	return string(chars)
}
