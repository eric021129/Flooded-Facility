game: main.o game.o gameplay_flood.o PuzzleInterface.o MediumPuzzle.o HardPuzzle.o
	g++ $^ -o $@

puzzle: mainPuzzle.o PuzzleInterface.o
	g++ $^ -o $@

mainPuzzle.o: mainPuzzle.cpp PuzzleInterface.h
	g++ -c $<

PuzzleInterface.o: PuzzleInterface.cpp PuzzleInterface.h
	g++ -c $<

MediumPuzzle.o: MediumPuzzle.cpp MediumPuzzle.h
	g++ -c $<

HardPuzzle.o: HardPuzzle.cpp HardPuzzle.h
	g++ -c $<

main.o: main.cpp game.h gameplay_flood.h
	g++ -c $<

game.o: game.cpp game.h gameplay_flood.h
	g++ -c $<

gameplay_flood.o: gameplay_flood.cpp gameplay_flood.h
	g++ -c $<

clean:
	rm -f game puzzle *.o

.PHONY: clean
